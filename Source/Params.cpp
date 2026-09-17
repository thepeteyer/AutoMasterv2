#include "Params.h"
#include <vector>
#include <memory>

namespace Param
{
    using Range = juce::NormalisableRange<float>;
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam = juce::AudioParameterBool;
    using ChoiceParam = juce::AudioParameterChoice;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { continuousAdaptive, 1 }, "Continuous / Adaptive", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { adaptiveSensitivity, 1 }, "Adaptive Sensitivity", Range { 0.0f, 1.0f }, 0.3f));

        // --- Stage 1: Input & Gain Staging ---
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { gainBypass, 1 }, "Gain Stage Bypass", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { gainInputTrim, 1 }, "Input Trim", Range { -24.0f, 24.0f, 0.01f }, 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { gainAutoMatch, 1 }, "Auto Gain Match", true));

        // --- Stage 2: EQ ---
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { eqBypass, 1 }, "EQ Bypass", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqLowShelfFreq, 1 }, "Low Shelf Freq",
            Range { 30.0f, 300.0f, 0.01f, 0.4f }, 80.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqLowShelfGain, 1 }, "Low Shelf Gain",
            Range { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqLowMidFreq, 1 }, "Low-Mid Freq",
            Range { 150.0f, 1200.0f, 0.01f, 0.4f }, 300.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqLowMidGain, 1 }, "Low-Mid Gain",
            Range { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqLowMidQ, 1 }, "Low-Mid Q",
            Range { 0.2f, 4.0f, 0.01f, 0.5f }, 0.7f));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqHighMidFreq, 1 }, "High-Mid Freq",
            Range { 800.0f, 8000.0f, 0.01f, 0.4f }, 3000.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqHighMidGain, 1 }, "High-Mid Gain",
            Range { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqHighMidQ, 1 }, "High-Mid Q",
            Range { 0.2f, 4.0f, 0.01f, 0.5f }, 0.7f));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqHighShelfFreq, 1 }, "High Shelf Freq",
            Range { 3000.0f, 16000.0f, 0.01f, 0.4f }, 9000.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqHighShelfGain, 1 }, "High Shelf Gain",
            Range { -12.0f, 12.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { eqTilt, 1 }, "Tilt",
            Range { -1.0f, 1.0f, 0.001f }, 0.0f));

        // --- Stage 3: Compression ---
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { compBypass, 1 }, "Compressor Bypass", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { compThreshold, 1 }, "Threshold",
            Range { -48.0f, 0.0f, 0.01f }, -18.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { compRatio, 1 }, "Ratio",
            Range { 1.0f, 10.0f, 0.01f, 0.5f }, 2.0f, juce::AudioParameterFloatAttributes().withLabel(":1")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { compAttack, 1 }, "Attack",
            Range { 0.5f, 100.0f, 0.01f, 0.4f }, 20.0f, juce::AudioParameterFloatAttributes().withLabel("ms")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { compRelease, 1 }, "Release",
            Range { 20.0f, 800.0f, 0.01f, 0.4f }, 150.0f, juce::AudioParameterFloatAttributes().withLabel("ms")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { compMakeup, 1 }, "Makeup Gain",
            Range { -6.0f, 18.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { compGlue, 1 }, "Glue",
            Range { 0.0f, 1.0f, 0.001f }, 0.3f));

        // --- Stage 4: Saturation ---
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { satBypass, 1 }, "Saturation Bypass", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { satDrive, 1 }, "Drive",
            Range { 0.0f, 1.0f, 0.001f }, 0.15f));
        p.push_back(std::make_unique<ChoiceParam>(juce::ParameterID { satCharacter, 1 }, "Character",
            juce::StringArray { "Tape", "Tube", "Transistor", "Clip" }, 0));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { satMix, 1 }, "Mix",
            Range { 0.0f, 100.0f, 0.1f }, 20.0f, juce::AudioParameterFloatAttributes().withLabel("%")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { satTone, 1 }, "Tone Tilt",
            Range { -1.0f, 1.0f, 0.001f }, 0.0f));

        // --- Stage 5: Stereo / Imaging ---
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { stereoBypass, 1 }, "Stereo Bypass", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { stereoLowWidth, 1 }, "Low Width",
            Range { 0.0f, 2.0f, 0.001f }, 0.15f));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { stereoLowXover, 1 }, "Low Crossover",
            Range { 60.0f, 300.0f, 0.01f, 0.5f }, 120.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { stereoMidWidth, 1 }, "Mid Width",
            Range { 0.0f, 2.0f, 0.001f }, 1.05f));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { stereoHighWidth, 1 }, "High Width",
            Range { 0.0f, 2.0f, 0.001f }, 1.1f));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { stereoHighXover, 1 }, "High Crossover",
            Range { 1500.0f, 10000.0f, 0.01f, 0.5f }, 4000.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { stereoWidthMacro, 1 }, "Width",
            Range { 0.0f, 2.0f, 0.001f }, 1.0f));

        // --- Stage 6: Limiter / Loudness ---
        p.push_back(std::make_unique<BoolParam>(juce::ParameterID { limBypass, 1 }, "Limiter Bypass", false));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { limCeiling, 1 }, "Ceiling",
            Range { -3.0f, 0.0f, 0.01f }, -1.0f, juce::AudioParameterFloatAttributes().withLabel("dBTP")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { limTargetLufs, 1 }, "Target Loudness",
            Range { -23.0f, -6.0f, 0.01f }, -14.0f, juce::AudioParameterFloatAttributes().withLabel("LUFS")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { limRelease, 1 }, "Release",
            Range { 10.0f, 500.0f, 0.01f, 0.4f }, 50.0f, juce::AudioParameterFloatAttributes().withLabel("ms")));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { limPunchVsLoud, 1 }, "Punch vs Loud",
            Range { 0.0f, 1.0f, 0.001f }, 0.5f));

        return { p.begin(), p.end() };
    }
}
