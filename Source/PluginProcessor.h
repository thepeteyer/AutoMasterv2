#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <memory>
#include "Params.h"
#include "analysis/AnalysisEngine.h"
#include "rules/RulesEngine.h"
#include "rules/ChainParameters.h"
#include "dsp/MasteringChain.h"

enum class TransportMode { Bypass, Learn, Apply, Freeze };

class AutoMasterAudioProcessor : public juce::AudioProcessor
{
public:
    AutoMasterAudioProcessor();
    ~AutoMasterAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // --- Transport --------------------------------------------------------
    TransportMode getTransportMode() const { return transportMode.load(); }
    void setTransportMode(TransportMode newMode);
    bool isAnalysisInProgress() const { return analysisInProgress.load(); }

    // --- AI suggestion access / per-tab reset ------------------------------
    const AnalysisResult& getLastAnalysisResult() const { return lastAnalysisResult; }
    const ChainParameters& getLastSuggestion() const { return lastSuggestion; }
    void resetStageToSuggestion(int stageIndex); // 0=Gain 1=EQ 2=Comp 3=Sat 4=Stereo 5=Limiter

    // --- Live metering for the UI ------------------------------------------
    MasteringChain& getChain() { return masteringChain; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void feedLearnRing(const juce::AudioBuffer<float>& buffer);
    void triggerAnalysisFromLearnBuffer();
    void applyChainParametersToApvts(const ChainParameters& c, float blend);
    void setParamSmoothOrSnap(const char* id, float nativeTarget, float blend);
    void timerAdaptiveTick();

    std::atomic<TransportMode> transportMode { TransportMode::Apply };
    std::atomic<bool> analysisInProgress { false };

    juce::AudioBuffer<float> learnRing;
    std::atomic<int> learnWritePos { 0 };
    std::atomic<int> learnSamplesWritten { 0 };
    static constexpr double maxLearnSeconds = 30.0;

    AnalysisEngine analysisEngine;
    std::unique_ptr<IRulesEngine> rulesEngine;
    AnalysisResult lastAnalysisResult;
    ChainParameters lastSuggestion;

    MasteringChain masteringChain;
    int secondsSinceLastAdapt = 0;

    class AdaptiveTimer : public juce::Timer
    {
    public:
        explicit AdaptiveTimer(AutoMasterAudioProcessor& ownerToUse) : owner(ownerToUse) {}
        void timerCallback() override { owner.timerAdaptiveTick(); }
    private:
        AutoMasterAudioProcessor& owner;
    };
    AdaptiveTimer adaptiveTimer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoMasterAudioProcessor)
};
