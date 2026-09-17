#pragma once
#include <JuceHeader.h>
#include <functional>
#include "AnalysisTypes.h"

// Runs a full analysis pass (spectral balance, LUFS, crest factor, stereo
// correlation, transient density) over a captured buffer of Learn-mode
// audio. The pass is offline/one-shot by design -- see LearnBuffer -- and
// runs on a background thread so it never touches the audio callback.
class AnalysisEngine : private juce::Thread
{
public:
    AnalysisEngine() : juce::Thread("AutoMaster Analysis") {}
    ~AnalysisEngine() override { stopThread(2000); }

    // Copies `buffer` (only `validSamples` of it) and analyses it on a
    // background thread; `onComplete` is called on the message thread.
    void analyseAsync(const juce::AudioBuffer<float>& buffer,
                       int validSamples,
                       double sampleRate,
                       std::function<void(AnalysisResult)> onComplete)
    {
        stopThread(2000);

        pending.setSize(buffer.getNumChannels(), validSamples, false, true, true);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            pending.copyFrom(ch, 0, buffer, ch, 0, validSamples);

        pendingSampleRate = sampleRate;
        callback = std::move(onComplete);

        startThread(juce::Thread::Priority::background);
    }

    static AnalysisResult analyse(const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    void run() override
    {
        auto result = analyse(pending, pendingSampleRate);
        if (callback)
            juce::MessageManager::callAsync([cb = callback, result] { cb(result); });
    }

    juce::AudioBuffer<float> pending;
    double pendingSampleRate = 44100.0;
    std::function<void(AnalysisResult)> callback;
};
