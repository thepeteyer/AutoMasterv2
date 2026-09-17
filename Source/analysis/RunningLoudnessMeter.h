#pragma once
#include "KWeighting.h"
#include <vector>

// Lightweight real-time LUFS-ish meter: K-weighted mean square with an
// exponential moving average (~3s time constant), one instance per monitored
// point in the chain. Cheap enough (2 biquads/channel) to run continuously,
// unlike the offline block-based analysis AnalysisEngine does for Learn mode.
// Used for live Auto Gain Match (input vs output loudness) and to drive the
// limiter's static target-loudness trim.
class RunningLoudnessMeter
{
public:
    void prepare(double sampleRate, int numChannels)
    {
        filters.resize((size_t) numChannels);
        for (auto& f : filters)
            f.prepare(sampleRate);

        // Exponential moving average coefficient for a ~3 second time constant.
        emaCoeff = (float) std::exp(-1.0 / (3.0 * sampleRate));
        meanSquare = 1.0e-9f;
        reset();
    }

    void reset()
    {
        for (auto& f : filters)
            f.reset();
        meanSquare = 1.0e-9f;
    }

    void process(const juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin(buffer.getNumChannels(), (int) filters.size());
        const int numSamples = buffer.getNumSamples();
        if (numCh == 0 || numSamples == 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float sumSq = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float w = filters[(size_t) ch].processSample(buffer.getSample(ch, i));
                sumSq += w * w;
            }
            sumSq /= (float) numCh;
            meanSquare = emaCoeff * meanSquare + (1.0f - emaCoeff) * sumSq;
        }
    }

    float getLufs() const
    {
        return -0.691f + 10.0f * std::log10(juce::jmax(meanSquare, 1.0e-12f));
    }

private:
    std::vector<KWeightingFilter> filters;
    float emaCoeff = 0.0f;
    float meanSquare = 1.0e-9f;
};
