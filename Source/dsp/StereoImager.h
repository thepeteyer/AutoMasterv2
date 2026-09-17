#pragma once
#include <JuceHeader.h>

// Mid/Side 3-band width control. Only the Side channel is band-split and
// scaled (Mid stays untouched, which keeps the mono sum stable regardless of
// width settings): the low band defaults near-mono, mid/high can be pushed
// wider. Two cascaded pairs of 4th-order Linkwitz-Riley filters do the
// split -- minimum-phase IIR, so this stage reports no added latency
// (unlike the true-peak oversampler in LimiterStage).
class StereoImager
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        // The Side signal is a single derived channel, independent of the
        // buffer's own channel count, so every filter here runs mono.
        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        for (auto* f : { &lowLP, &lowHP, &midLP, &midHP })
            f->prepare(monoSpec);

        lowLP.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
        lowHP.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
        midLP.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
        midHP.setType(juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
    }

    void reset()
    {
        for (auto* f : { &lowLP, &lowHP, &midLP, &midHP })
            f->reset();
    }

    void setCrossovers(float lowHz, float highHz)
    {
        lowLP.setCutoffFrequency(lowHz);
        lowHP.setCutoffFrequency(lowHz);
        midLP.setCutoffFrequency(highHz);
        midHP.setCutoffFrequency(highHz);
    }

    // Processes an interleaved stereo buffer (channel 0 = L, 1 = R) in place.
    void process(juce::AudioBuffer<float>& buffer, float lowWidth, float midWidth, float highWidth, float widthMacro)
    {
        if (buffer.getNumChannels() < 2)
            return;

        const int numSamples = buffer.getNumSamples();
        auto* l = buffer.getWritePointer(0);
        auto* r = buffer.getWritePointer(1);

        const float lw = lowWidth * widthMacro;
        const float mw = midWidth * widthMacro;
        const float hw = highWidth * widthMacro;

        for (int i = 0; i < numSamples; ++i)
        {
            const float mid = 0.5f * (l[i] + r[i]);
            const float side = 0.5f * (l[i] - r[i]);

            const float low = lowLP.processSample(0, side);
            const float aboveLow = lowHP.processSample(0, side);
            const float mid_ = midLP.processSample(0, aboveLow);
            const float high = midHP.processSample(0, aboveLow);

            const float sideOut = low * lw + mid_ * mw + high * hw;

            l[i] = mid + sideOut;
            r[i] = mid - sideOut;
        }
    }

private:
    juce::dsp::LinkwitzRileyFilter<float> lowLP, lowHP, midLP, midHP;
};
