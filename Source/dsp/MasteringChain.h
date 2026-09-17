#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "../Params.h"
#include "../rules/ChainParameters.h"
#include "../analysis/RunningLoudnessMeter.h"
#include "StereoImager.h"
#include "Saturator.h"

// Owns the whole static signal chain: Gain -> EQ -> Compression ->
// Saturation -> Stereo Imaging -> Limiter, in that order. Every stage reads
// its parameters from the live APVTS each block (or from a frozen snapshot,
// see setFrozen) rather than caching a ChainParameters permanently, so
// manual knob tweaks, host automation and "Reset to AI suggestion" all just
// work through the one normal parameter path. A handful of block-rate
// smoothers avoid zipper noise on the parameters most likely to jump
// (AI-applied resets, fast automation).
class MasteringChain
{
public:
    explicit MasteringChain(juce::AudioProcessorValueTreeState& stateToUse) : apvts(stateToUse) {}

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const { return latencySamples; }

    // Freezing captures the current knob positions and processes from that
    // snapshot instead of live APVTS reads, so neither host automation nor
    // an accidental knob bump can change the sound until unfrozen.
    void setFrozen(bool shouldFreeze);

    float getInputLufs() const { return inputMeter.getLufs(); }
    float getOutputLufs() const { return outputMeter.getLufs(); }
    float getOutputPeakDb() const { return outputPeakDb.load(); }
    float getOutputCorrelation() const { return outputCorrelation.load(); }
    float getCompressorGainReductionDb() const { return compGainReductionDb.load(); }
    float getLimiterGainReductionDb() const { return limGainReductionDb.load(); }
    float getAutoGainMatchTrimDb() const { return autoGainTrimDb.load(); }

private:
    ChainParameters gatherLiveParams() const;
    bool isBypassed(const char* paramId) const;
    void snapAllSmoothers(const ChainParameters& cp);

    void processGain(juce::AudioBuffer<float>& buffer, const ChainParameters& cp);
    void processEq(juce::AudioBuffer<float>& buffer, const ChainParameters& cp);
    void processComp(juce::AudioBuffer<float>& buffer, const ChainParameters& cp);
    void processSat(juce::AudioBuffer<float>& buffer, const ChainParameters& cp);
    void processStereo(juce::AudioBuffer<float>& buffer, const ChainParameters& cp);
    void processLimiter(juce::AudioBuffer<float>& buffer, const ChainParameters& cp);

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate = 44100.0;
    int latencySamples = 0;
    float blockSmoothCoeff = 0.3f;

    bool frozen = false;
    ChainParameters frozenSnapshot;

    // --- EQ (ProcessorDuplicator handles per-channel state for us) ------
    using IIRDuplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    IIRDuplicator eqLowShelf, eqLowMid, eqHighMid, eqHighShelf;
    IIRDuplicator satToneLow, satToneHigh;

    // --- Compression ------------------------------------------------------
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> compMakeupGain;

    // Scratch buffer for the saturation dry/wet blend (pre-sized in prepare()
    // to avoid audio-thread allocation).
    juce::AudioBuffer<float> satDryBuffer;

    // --- Stereo imaging -----------------------------------------------
    StereoImager stereoImager;

    // --- Limiter: true-peak via 2x oversampling ------------------------
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Gain<float> limiterTargetTrim;

    // --- Metering / auto gain match ------------------------------------
    RunningLoudnessMeter inputMeter, outputMeter;
    std::atomic<float> outputPeakDb { -60.0f };
    std::atomic<float> outputCorrelation { 1.0f };
    std::atomic<float> compGainReductionDb { 0.0f };
    std::atomic<float> limGainReductionDb { 0.0f };
    std::atomic<float> autoGainTrimDb { 0.0f };
    float smoothedAutoGainTrimDb = 0.0f;

    // Block-rate smoothers for the parameters most likely to jump.
    struct Smoother { float value = 0.0f, target = 0.0f;
        void snap(float v) { value = target = v; }
        float tick(float coeff) { value += (target - value) * coeff; return value; } };

    Smoother smInputTrim, smEqLowShelfGain, smEqLowMidGain, smEqHighMidGain, smEqHighShelfGain, smEqTilt;
    Smoother smCompThreshold, smCompRatio, smCompAttack, smCompRelease, smCompMakeup;
    Smoother smSatDrive, smSatMix, smSatTone;
    Smoother smStereoLowWidth, smStereoMidWidth, smStereoHighWidth, smStereoWidthMacro;
    Smoother smLimCeiling, smLimTargetLufs, smLimRelease;
    bool smoothersInitialised = false;
};
