#include "AnalysisEngine.h"
#include "KWeighting.h"
#include <cmath>

namespace
{
    // Band edges in Hz: sub, bass, lowMid, mid, highMid, air.
    constexpr std::array<float, 7> kBandEdges { 20.f, 60.f, 150.f, 500.f, 2000.f, 6000.f, 20000.f };

    float linToDb(float lin) { return 20.0f * std::log10(juce::jmax(lin, 1.0e-9f)); }
}

AnalysisResult AnalysisEngine::analyse(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    AnalysisResult result;
    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numCh == 0 || numSamples < (int) sampleRate) // require at least ~1s
        return result;

    result.secondsAnalyzed = numSamples / sampleRate;

    // --- Peak / RMS / crest factor -----------------------------------
    float peak = 0.0f;
    double sumSq = 0.0;
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto* d = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            peak = juce::jmax(peak, std::abs(d[i]));
            sumSq += (double) d[i] * (double) d[i];
        }
    }
    const float rms = (float) std::sqrt(sumSq / ((double) numSamples * numCh));
    result.peakDb = linToDb(peak);
    result.rmsDb = linToDb(rms);
    result.crestFactorDb = result.peakDb - result.rmsDb;

    // --- Integrated LUFS (K-weighted mean square, ungated -- see docs) --
    {
        std::vector<KWeightingFilter> filters((size_t) numCh);
        for (auto& f : filters)
            f.prepare(sampleRate);

        double weightedSumSq = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            float frameSq = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float w = filters[(size_t) ch].processSample(buffer.getSample(ch, i));
                frameSq += w * w;
            }
            weightedSumSq += frameSq / numCh;
        }
        const double meanSq = weightedSumSq / numSamples;
        result.integratedLufs = -0.691f + 10.0f * std::log10((float) juce::jmax(meanSq, 1.0e-12));
    }

    // --- Stereo correlation -------------------------------------------
    if (numCh >= 2)
    {
        const auto* l = buffer.getReadPointer(0);
        const auto* r = buffer.getReadPointer(1);
        double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            sumLR += (double) l[i] * r[i];
            sumLL += (double) l[i] * l[i];
            sumRR += (double) r[i] * r[i];
        }
        const double denom = std::sqrt(sumLL * sumRR);
        result.stereoCorrelation = denom > 1.0e-9 ? (float) (sumLR / denom) : 1.0f;
    }
    else
    {
        result.stereoCorrelation = 1.0f;
    }

    // --- Spectral balance via FFT --------------------------------------
    {
        constexpr int fftOrder = 12;
        constexpr int fftSize = 1 << fftOrder;
        constexpr int hop = fftSize / 2;

        juce::dsp::FFT fft(fftOrder);
        juce::dsp::WindowingFunction<float> window(fftSize, juce::dsp::WindowingFunction<float>::hann);

        std::array<double, 6> bandPowerSum {};
        std::array<int, 6> bandBinCount {};
        int numFrames = 0;

        std::vector<float> mono((size_t) fftSize * 2, 0.0f);
        std::vector<float> frame((size_t) fftSize, 0.0f);

        for (int start = 0; start + fftSize <= numSamples; start += hop)
        {
            for (int i = 0; i < fftSize; ++i)
            {
                float s = 0.0f;
                for (int ch = 0; ch < numCh; ++ch)
                    s += buffer.getSample(ch, start + i);
                frame[(size_t) i] = s / (float) numCh;
            }

            window.multiplyWithWindowingTable(frame.data(), fftSize);
            std::fill(mono.begin(), mono.end(), 0.0f);
            std::copy(frame.begin(), frame.end(), mono.begin());

            fft.performRealOnlyForwardTransform(mono.data());

            const int numBins = fftSize / 2;
            for (int bin = 1; bin < numBins; ++bin)
            {
                const float re = mono[(size_t) bin * 2];
                const float im = mono[(size_t) bin * 2 + 1];
                const float power = re * re + im * im;
                const float freq = (float) bin * (float) sampleRate / (float) fftSize;

                for (size_t b = 0; b < 6; ++b)
                {
                    if (freq >= kBandEdges[b] && freq < kBandEdges[b + 1])
                    {
                        bandPowerSum[b] += power;
                        bandBinCount[b] += 1;
                        break;
                    }
                }
            }
            ++numFrames;
        }

        for (size_t b = 0; b < 6; ++b)
        {
            const double avgPower = bandBinCount[b] > 0 && numFrames > 0
                ? bandPowerSum[b] / (double) bandBinCount[b] / (double) numFrames
                : 1.0e-9;
            result.bandEnergyDb[b] = 10.0f * std::log10((float) juce::jmax(avgPower, 1.0e-9));
        }

        // Linear regression of band dB vs log2(center frequency) -> dB/octave tilt.
        double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
        for (size_t b = 0; b < 6; ++b)
        {
            const float centre = std::sqrt(kBandEdges[b] * kBandEdges[b + 1]);
            const double x = std::log2(centre);
            const double y = result.bandEnergyDb[b];
            sumX += x; sumY += y; sumXY += x * y; sumXX += x * x;
        }
        const double n = 6.0;
        const double denom = n * sumXX - sumX * sumX;
        result.spectralTiltDbPerOctave = denom > 1.0e-9 ? (float) ((n * sumXY - sumX * sumY) / denom) : 0.0f;
    }

    // --- Transient density: onset rate from a short-term RMS envelope --
    {
        const int winSize = juce::jmax(1, (int) (sampleRate * 0.005));  // 5ms
        const int hopSize = juce::jmax(1, (int) (sampleRate * 0.010));  // 10ms

        std::vector<float> envelope;
        for (int start = 0; start + winSize <= numSamples; start += hopSize)
        {
            double sq = 0.0;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const auto* d = buffer.getReadPointer(ch);
                for (int i = 0; i < winSize; ++i)
                    sq += (double) d[start + i] * d[start + i];
            }
            envelope.push_back((float) std::sqrt(sq / (winSize * numCh)));
        }

        int onsets = 0;
        for (size_t i = 1; i + 1 < envelope.size(); ++i)
        {
            const float prevDb = linToDb(envelope[i - 1]);
            const float curDb = linToDb(envelope[i]);
            if (curDb - prevDb > 3.0f) // rising edge >3dB between hops
                ++onsets;
        }

        const double seconds = result.secondsAnalyzed;
        const float onsetsPerSecond = seconds > 0.0 ? (float) (onsets / seconds) : 0.0f;
        result.transientDensity = juce::jlimit(0.0f, 1.0f, onsetsPerSecond / 8.0f);
    }

    result.valid = true;
    return result;
}
