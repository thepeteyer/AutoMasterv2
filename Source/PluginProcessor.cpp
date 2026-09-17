#include "PluginProcessor.h"
#include "PluginEditor.h"

AutoMasterAudioProcessor::AutoMasterAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", Param::createLayout()),
      rulesEngine(std::make_unique<DefaultRulesEngine>()),
      masteringChain(apvts)
{
    adaptiveTimer.startTimerHz(1); // 1Hz housekeeping; timerAdaptiveTick self-paces the real analysis cadence
}

AutoMasterAudioProcessor::~AutoMasterAudioProcessor()
{
    adaptiveTimer.stopTimer();
}

void AutoMasterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int numChannels = juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) juce::jmax(1, samplesPerBlock);
    spec.numChannels = (juce::uint32) juce::jmax(1, numChannels);

    masteringChain.prepare(spec);
    setLatencySamples(masteringChain.getLatencySamples());

    learnRing.setSize(numChannels, (int) (sampleRate * maxLearnSeconds), false, true, true);
    learnRing.clear();
    learnWritePos = 0;
    learnSamplesWritten = 0;
}

bool AutoMasterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;
    if (in.isDisabled())
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void AutoMasterAudioProcessor::feedLearnRing(const juce::AudioBuffer<float>& buffer)
{
    const int numCh = juce::jmin(buffer.getNumChannels(), learnRing.getNumChannels());
    const int numSamples = buffer.getNumSamples();
    const int capacity = learnRing.getNumSamples();
    if (capacity == 0 || numCh == 0)
        return;

    int pos = learnWritePos.load();
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
            learnRing.setSample(ch, pos, buffer.getSample(ch, i));
        pos = (pos + 1) % capacity;
    }
    learnWritePos.store(pos);
    learnSamplesWritten.store(juce::jmin(capacity, learnSamplesWritten.load() + numSamples));
}

void AutoMasterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const auto mode = transportMode.load();
    const bool continuousAdaptive = apvts.getRawParameterValue(Param::continuousAdaptive)->load() > 0.5f;

    if (mode == TransportMode::Learn || (continuousAdaptive && mode != TransportMode::Bypass))
        feedLearnRing(buffer);

    if (mode == TransportMode::Bypass || mode == TransportMode::Learn)
        return; // listen-through: no processing, analysis (if any) happens on the dry signal above

    masteringChain.process(buffer);
}

void AutoMasterAudioProcessor::setTransportMode(TransportMode newMode)
{
    const auto oldMode = transportMode.load();

    if (oldMode == TransportMode::Learn && newMode != TransportMode::Learn && learnSamplesWritten.load() > 0)
        triggerAnalysisFromLearnBuffer();

    if (newMode == TransportMode::Learn)
    {
        learnSamplesWritten.store(0);
        learnWritePos.store(0);
    }

    masteringChain.setFrozen(newMode == TransportMode::Freeze);
    transportMode.store(newMode);
}

void AutoMasterAudioProcessor::triggerAnalysisFromLearnBuffer()
{
    const int capacity = learnRing.getNumSamples();
    const int written = juce::jmin(learnSamplesWritten.load(), capacity);
    if (capacity == 0 || written < (int) getSampleRate()) // require at least ~1s
        return;

    const int numCh = learnRing.getNumChannels();
    juce::AudioBuffer<float> linear(numCh, written);
    const int startPos = ((learnWritePos.load() - written) % capacity + capacity) % capacity;

    for (int i = 0; i < written; ++i)
    {
        const int srcIdx = (startPos + i) % capacity;
        for (int ch = 0; ch < numCh; ++ch)
            linear.setSample(ch, i, learnRing.getSample(ch, srcIdx));
    }

    analysisInProgress.store(true);
    analysisEngine.analyseAsync(linear, written, getSampleRate(), [this](AnalysisResult result)
    {
        lastAnalysisResult = result;
        if (result.valid)
        {
            lastSuggestion = rulesEngine->derive(result);
            applyChainParametersToApvts(lastSuggestion, 1.0f);
        }
        analysisInProgress.store(false);
        transportMode.store(TransportMode::Apply);
    });
}

void AutoMasterAudioProcessor::timerAdaptiveTick()
{
    const bool continuousAdaptive = apvts.getRawParameterValue(Param::continuousAdaptive)->load() > 0.5f;
    const auto mode = transportMode.load();

    if (!continuousAdaptive || mode != TransportMode::Apply || analysisInProgress.load())
    {
        secondsSinceLastAdapt = 0;
        return;
    }

    if (++secondsSinceLastAdapt < 8)
        return;
    secondsSinceLastAdapt = 0;

    const int capacity = learnRing.getNumSamples();
    const int written = juce::jmin(learnSamplesWritten.load(), capacity);
    if (written < (int) (getSampleRate() * 5.0))
        return;

    const float sensitivity = apvts.getRawParameterValue(Param::adaptiveSensitivity)->load();

    const int numCh = learnRing.getNumChannels();
    juce::AudioBuffer<float> linear(numCh, written);
    const int startPos = ((learnWritePos.load() - written) % capacity + capacity) % capacity;
    for (int i = 0; i < written; ++i)
    {
        const int srcIdx = (startPos + i) % capacity;
        for (int ch = 0; ch < numCh; ++ch)
            linear.setSample(ch, i, learnRing.getSample(ch, srcIdx));
    }

    analysisInProgress.store(true);
    analysisEngine.analyseAsync(linear, written, getSampleRate(), [this, sensitivity](AnalysisResult result)
    {
        lastAnalysisResult = result;
        if (result.valid)
        {
            lastSuggestion = rulesEngine->derive(result);
            // Gentle nudge, not a snap -- sensitivity scales how far we move per adaptation tick.
            applyChainParametersToApvts(lastSuggestion, juce::jlimit(0.0f, 1.0f, sensitivity) * 0.3f);
        }
        analysisInProgress.store(false);
    });
}

void AutoMasterAudioProcessor::setParamSmoothOrSnap(const char* id, float nativeTarget, float blend)
{
    auto* param = apvts.getParameter(id);
    if (param == nullptr)
        return;
    const float currentNative = apvts.getRawParameterValue(id)->load();
    const float newNative = currentNative + (nativeTarget - currentNative) * blend;
    param->setValueNotifyingHost(param->convertTo0to1(newNative));
}

void AutoMasterAudioProcessor::applyChainParametersToApvts(const ChainParameters& c, float blend)
{
    setParamSmoothOrSnap(Param::gainInputTrim, c.gain.inputTrimDb, blend);
    if (auto* p = apvts.getParameter(Param::gainAutoMatch))
        p->setValueNotifyingHost(c.gain.autoGainMatch ? 1.0f : 0.0f);

    setParamSmoothOrSnap(Param::eqLowShelfFreq, c.eq.lowShelfFreq, blend);
    setParamSmoothOrSnap(Param::eqLowShelfGain, c.eq.lowShelfGainDb, blend);
    setParamSmoothOrSnap(Param::eqLowMidFreq, c.eq.lowMidFreq, blend);
    setParamSmoothOrSnap(Param::eqLowMidGain, c.eq.lowMidGainDb, blend);
    setParamSmoothOrSnap(Param::eqLowMidQ, c.eq.lowMidQ, blend);
    setParamSmoothOrSnap(Param::eqHighMidFreq, c.eq.highMidFreq, blend);
    setParamSmoothOrSnap(Param::eqHighMidGain, c.eq.highMidGainDb, blend);
    setParamSmoothOrSnap(Param::eqHighMidQ, c.eq.highMidQ, blend);
    setParamSmoothOrSnap(Param::eqHighShelfFreq, c.eq.highShelfFreq, blend);
    setParamSmoothOrSnap(Param::eqHighShelfGain, c.eq.highShelfGainDb, blend);
    setParamSmoothOrSnap(Param::eqTilt, c.eq.tilt, blend);

    setParamSmoothOrSnap(Param::compThreshold, c.comp.thresholdDb, blend);
    setParamSmoothOrSnap(Param::compRatio, c.comp.ratio, blend);
    setParamSmoothOrSnap(Param::compAttack, c.comp.attackMs, blend);
    setParamSmoothOrSnap(Param::compRelease, c.comp.releaseMs, blend);
    setParamSmoothOrSnap(Param::compMakeup, c.comp.makeupDb, blend);
    setParamSmoothOrSnap(Param::compGlue, c.comp.glue, blend);

    setParamSmoothOrSnap(Param::satDrive, c.sat.drive, blend);
    if (auto* p = apvts.getParameter(Param::satCharacter))
        p->setValueNotifyingHost(p->convertTo0to1((float) c.sat.character));
    setParamSmoothOrSnap(Param::satMix, c.sat.mix, blend);
    setParamSmoothOrSnap(Param::satTone, c.sat.toneTilt, blend);

    setParamSmoothOrSnap(Param::stereoLowWidth, c.stereo.lowWidth, blend);
    setParamSmoothOrSnap(Param::stereoLowXover, c.stereo.lowCrossoverHz, blend);
    setParamSmoothOrSnap(Param::stereoMidWidth, c.stereo.midWidth, blend);
    setParamSmoothOrSnap(Param::stereoHighWidth, c.stereo.highWidth, blend);
    setParamSmoothOrSnap(Param::stereoHighXover, c.stereo.highCrossoverHz, blend);
    setParamSmoothOrSnap(Param::stereoWidthMacro, c.stereo.widthMacro, blend);

    setParamSmoothOrSnap(Param::limCeiling, c.lim.ceilingDb, blend);
    setParamSmoothOrSnap(Param::limTargetLufs, c.lim.targetLufs, blend);
    setParamSmoothOrSnap(Param::limRelease, c.lim.releaseMs, blend);
    setParamSmoothOrSnap(Param::limPunchVsLoud, c.lim.punchVsLoud, blend);
}

void AutoMasterAudioProcessor::resetStageToSuggestion(int stageIndex)
{
    const auto& c = lastSuggestion;
    constexpr float snap = 1.0f;

    switch (stageIndex)
    {
        case 0:
            setParamSmoothOrSnap(Param::gainInputTrim, c.gain.inputTrimDb, snap);
            if (auto* p = apvts.getParameter(Param::gainAutoMatch))
                p->setValueNotifyingHost(c.gain.autoGainMatch ? 1.0f : 0.0f);
            break;
        case 1:
            setParamSmoothOrSnap(Param::eqLowShelfFreq, c.eq.lowShelfFreq, snap);
            setParamSmoothOrSnap(Param::eqLowShelfGain, c.eq.lowShelfGainDb, snap);
            setParamSmoothOrSnap(Param::eqLowMidFreq, c.eq.lowMidFreq, snap);
            setParamSmoothOrSnap(Param::eqLowMidGain, c.eq.lowMidGainDb, snap);
            setParamSmoothOrSnap(Param::eqLowMidQ, c.eq.lowMidQ, snap);
            setParamSmoothOrSnap(Param::eqHighMidFreq, c.eq.highMidFreq, snap);
            setParamSmoothOrSnap(Param::eqHighMidGain, c.eq.highMidGainDb, snap);
            setParamSmoothOrSnap(Param::eqHighMidQ, c.eq.highMidQ, snap);
            setParamSmoothOrSnap(Param::eqHighShelfFreq, c.eq.highShelfFreq, snap);
            setParamSmoothOrSnap(Param::eqHighShelfGain, c.eq.highShelfGainDb, snap);
            setParamSmoothOrSnap(Param::eqTilt, c.eq.tilt, snap);
            break;
        case 2:
            setParamSmoothOrSnap(Param::compThreshold, c.comp.thresholdDb, snap);
            setParamSmoothOrSnap(Param::compRatio, c.comp.ratio, snap);
            setParamSmoothOrSnap(Param::compAttack, c.comp.attackMs, snap);
            setParamSmoothOrSnap(Param::compRelease, c.comp.releaseMs, snap);
            setParamSmoothOrSnap(Param::compMakeup, c.comp.makeupDb, snap);
            setParamSmoothOrSnap(Param::compGlue, c.comp.glue, snap);
            break;
        case 3:
            setParamSmoothOrSnap(Param::satDrive, c.sat.drive, snap);
            if (auto* p = apvts.getParameter(Param::satCharacter))
                p->setValueNotifyingHost(p->convertTo0to1((float) c.sat.character));
            setParamSmoothOrSnap(Param::satMix, c.sat.mix, snap);
            setParamSmoothOrSnap(Param::satTone, c.sat.toneTilt, snap);
            break;
        case 4:
            setParamSmoothOrSnap(Param::stereoLowWidth, c.stereo.lowWidth, snap);
            setParamSmoothOrSnap(Param::stereoLowXover, c.stereo.lowCrossoverHz, snap);
            setParamSmoothOrSnap(Param::stereoMidWidth, c.stereo.midWidth, snap);
            setParamSmoothOrSnap(Param::stereoHighWidth, c.stereo.highWidth, snap);
            setParamSmoothOrSnap(Param::stereoHighXover, c.stereo.highCrossoverHz, snap);
            setParamSmoothOrSnap(Param::stereoWidthMacro, c.stereo.widthMacro, snap);
            break;
        case 5:
            setParamSmoothOrSnap(Param::limCeiling, c.lim.ceilingDb, snap);
            setParamSmoothOrSnap(Param::limTargetLufs, c.lim.targetLufs, snap);
            setParamSmoothOrSnap(Param::limRelease, c.lim.releaseMs, snap);
            setParamSmoothOrSnap(Param::limPunchVsLoud, c.lim.punchVsLoud, snap);
            break;
        default: break;
    }
}

void AutoMasterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("transportMode", (int) transportMode.load(), nullptr);
    state.setProperty("analysisValid", lastAnalysisResult.valid, nullptr);
    state.setProperty("analysisLufs", lastAnalysisResult.integratedLufs, nullptr);
    state.setProperty("analysisPeakDb", lastAnalysisResult.peakDb, nullptr);
    state.setProperty("analysisRmsDb", lastAnalysisResult.rmsDb, nullptr);
    state.setProperty("analysisCrestDb", lastAnalysisResult.crestFactorDb, nullptr);
    state.setProperty("analysisCorrelation", lastAnalysisResult.stereoCorrelation, nullptr);
    state.setProperty("analysisTilt", lastAnalysisResult.spectralTiltDbPerOctave, nullptr);
    state.setProperty("analysisTransientDensity", lastAnalysisResult.transientDensity, nullptr);
    state.setProperty("analysisSeconds", lastAnalysisResult.secondsAnalyzed, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AutoMasterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return;

    auto newState = juce::ValueTree::fromXml(*xml);
    apvts.replaceState(newState);

    transportMode.store((TransportMode) (int) newState.getProperty("transportMode", (int) TransportMode::Apply));
    masteringChain.setFrozen(transportMode.load() == TransportMode::Freeze);

    lastAnalysisResult.valid = newState.getProperty("analysisValid", false);
    lastAnalysisResult.integratedLufs = newState.getProperty("analysisLufs", -23.0f);
    lastAnalysisResult.peakDb = newState.getProperty("analysisPeakDb", -60.0f);
    lastAnalysisResult.rmsDb = newState.getProperty("analysisRmsDb", -60.0f);
    lastAnalysisResult.crestFactorDb = newState.getProperty("analysisCrestDb", 10.0f);
    lastAnalysisResult.stereoCorrelation = newState.getProperty("analysisCorrelation", 1.0f);
    lastAnalysisResult.spectralTiltDbPerOctave = newState.getProperty("analysisTilt", 0.0f);
    lastAnalysisResult.transientDensity = newState.getProperty("analysisTransientDensity", 0.5f);
    lastAnalysisResult.secondsAnalyzed = newState.getProperty("analysisSeconds", 0.0);

    if (lastAnalysisResult.valid)
        lastSuggestion = rulesEngine->derive(lastAnalysisResult);
}

juce::AudioProcessorEditor* AutoMasterAudioProcessor::createEditor()
{
    return new AutoMasterAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoMasterAudioProcessor();
}
