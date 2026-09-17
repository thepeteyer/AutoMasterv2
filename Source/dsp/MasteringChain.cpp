#include "MasteringChain.h"
#include <cmath>

namespace
{
    float bufferPeakDb(const juce::AudioBuffer<float>& b)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            peak = juce::jmax(peak, b.getMagnitude(ch, 0, b.getNumSamples()));
        return juce::Decibels::gainToDecibels(peak, -100.0f);
    }

    float bufferRmsDb(const juce::AudioBuffer<float>& b)
    {
        float sumSq = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            sumSq += b.getRMSLevel(ch, 0, b.getNumSamples()) * b.getRMSLevel(ch, 0, b.getNumSamples());
        const float rms = std::sqrt(sumSq / juce::jmax(1, b.getNumChannels()));
        return juce::Decibels::gainToDecibels(rms, -100.0f);
    }
}

void MasteringChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    eqLowShelf.prepare(spec);
    eqLowMid.prepare(spec);
    eqHighMid.prepare(spec);
    eqHighShelf.prepare(spec);
    satToneLow.prepare(spec);
    satToneHigh.prepare(spec);

    compressor.prepare(spec);
    compMakeupGain.prepare(spec);

    stereoImager.prepare(spec);

    satDryBuffer.setSize((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
    oversampling->initProcessing(spec.maximumBlockSize);
    latencySamples = (int) oversampling->getLatencyInSamples();

    auto oversampledSpec = spec;
    oversampledSpec.sampleRate = spec.sampleRate * 2.0;
    oversampledSpec.maximumBlockSize = spec.maximumBlockSize * 2;
    limiter.prepare(oversampledSpec);
    limiterTargetTrim.prepare(spec);

    inputMeter.prepare(spec.sampleRate, (int) spec.numChannels);
    outputMeter.prepare(spec.sampleRate, (int) spec.numChannels);

    reset();

    snapAllSmoothers(gatherLiveParams());
    smoothersInitialised = true;
}

void MasteringChain::reset()
{
    eqLowShelf.reset();
    eqLowMid.reset();
    eqHighMid.reset();
    eqHighShelf.reset();
    satToneLow.reset();
    satToneHigh.reset();
    compressor.reset();
    compMakeupGain.reset();
    stereoImager.reset();
    if (oversampling != nullptr)
        oversampling->reset();
    limiter.reset();
    limiterTargetTrim.reset();
    inputMeter.reset();
    outputMeter.reset();
    smoothedAutoGainTrimDb = 0.0f;
}

void MasteringChain::setFrozen(bool shouldFreeze)
{
    if (shouldFreeze && !frozen)
        frozenSnapshot = gatherLiveParams();
    frozen = shouldFreeze;
}

ChainParameters MasteringChain::gatherLiveParams() const
{
    ChainParameters cp;
    auto v = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };

    cp.gain.inputTrimDb = v(Param::gainInputTrim);
    cp.gain.autoGainMatch = v(Param::gainAutoMatch) > 0.5f;

    cp.eq.lowShelfFreq = v(Param::eqLowShelfFreq);
    cp.eq.lowShelfGainDb = v(Param::eqLowShelfGain);
    cp.eq.lowMidFreq = v(Param::eqLowMidFreq);
    cp.eq.lowMidGainDb = v(Param::eqLowMidGain);
    cp.eq.lowMidQ = v(Param::eqLowMidQ);
    cp.eq.highMidFreq = v(Param::eqHighMidFreq);
    cp.eq.highMidGainDb = v(Param::eqHighMidGain);
    cp.eq.highMidQ = v(Param::eqHighMidQ);
    cp.eq.highShelfFreq = v(Param::eqHighShelfFreq);
    cp.eq.highShelfGainDb = v(Param::eqHighShelfGain);
    cp.eq.tilt = v(Param::eqTilt);

    cp.comp.thresholdDb = v(Param::compThreshold);
    cp.comp.ratio = v(Param::compRatio);
    cp.comp.attackMs = v(Param::compAttack);
    cp.comp.releaseMs = v(Param::compRelease);
    cp.comp.makeupDb = v(Param::compMakeup);
    cp.comp.glue = v(Param::compGlue);

    cp.sat.drive = v(Param::satDrive);
    cp.sat.character = (int) v(Param::satCharacter);
    cp.sat.mix = v(Param::satMix);
    cp.sat.toneTilt = v(Param::satTone);

    cp.stereo.lowWidth = v(Param::stereoLowWidth);
    cp.stereo.lowCrossoverHz = v(Param::stereoLowXover);
    cp.stereo.midWidth = v(Param::stereoMidWidth);
    cp.stereo.highWidth = v(Param::stereoHighWidth);
    cp.stereo.highCrossoverHz = v(Param::stereoHighXover);
    cp.stereo.widthMacro = v(Param::stereoWidthMacro);

    cp.lim.ceilingDb = v(Param::limCeiling);
    cp.lim.targetLufs = v(Param::limTargetLufs);
    cp.lim.releaseMs = v(Param::limRelease);
    cp.lim.punchVsLoud = v(Param::limPunchVsLoud);

    return cp;
}

bool MasteringChain::isBypassed(const char* paramId) const
{
    return apvts.getRawParameterValue(paramId)->load() > 0.5f;
}

void MasteringChain::snapAllSmoothers(const ChainParameters& cp)
{
    smInputTrim.snap(cp.gain.inputTrimDb);
    smEqLowShelfGain.snap(cp.eq.lowShelfGainDb);
    smEqLowMidGain.snap(cp.eq.lowMidGainDb);
    smEqHighMidGain.snap(cp.eq.highMidGainDb);
    smEqHighShelfGain.snap(cp.eq.highShelfGainDb);
    smEqTilt.snap(cp.eq.tilt);
    smCompThreshold.snap(cp.comp.thresholdDb);
    smCompRatio.snap(cp.comp.ratio);
    smCompAttack.snap(cp.comp.attackMs);
    smCompRelease.snap(cp.comp.releaseMs);
    smCompMakeup.snap(cp.comp.makeupDb);
    smSatDrive.snap(cp.sat.drive);
    smSatMix.snap(cp.sat.mix);
    smSatTone.snap(cp.sat.toneTilt);
    smStereoLowWidth.snap(cp.stereo.lowWidth);
    smStereoMidWidth.snap(cp.stereo.midWidth);
    smStereoHighWidth.snap(cp.stereo.highWidth);
    smStereoWidthMacro.snap(cp.stereo.widthMacro);
    smLimCeiling.snap(cp.lim.ceilingDb);
    smLimTargetLufs.snap(cp.lim.targetLufs);
    smLimRelease.snap(cp.lim.releaseMs);
}

void MasteringChain::process(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() == 0)
        return;

    const ChainParameters cp = frozen ? frozenSnapshot : gatherLiveParams();

    const float blockSeconds = (float) buffer.getNumSamples() / (float) sampleRate;
    blockSmoothCoeff = 1.0f - std::exp(-blockSeconds / 0.05f);

    if (!smoothersInitialised)
    {
        snapAllSmoothers(cp);
        smoothersInitialised = true;
    }

    smInputTrim.target = cp.gain.inputTrimDb;
    smEqLowShelfGain.target = cp.eq.lowShelfGainDb;
    smEqLowMidGain.target = cp.eq.lowMidGainDb;
    smEqHighMidGain.target = cp.eq.highMidGainDb;
    smEqHighShelfGain.target = cp.eq.highShelfGainDb;
    smEqTilt.target = cp.eq.tilt;
    smCompThreshold.target = cp.comp.thresholdDb;
    smCompRatio.target = cp.comp.ratio;
    smCompAttack.target = cp.comp.attackMs;
    smCompRelease.target = cp.comp.releaseMs;
    smCompMakeup.target = cp.comp.makeupDb;
    smSatDrive.target = cp.sat.drive;
    smSatMix.target = cp.sat.mix;
    smSatTone.target = cp.sat.toneTilt;
    smStereoLowWidth.target = cp.stereo.lowWidth;
    smStereoMidWidth.target = cp.stereo.midWidth;
    smStereoHighWidth.target = cp.stereo.highWidth;
    smStereoWidthMacro.target = cp.stereo.widthMacro;
    smLimCeiling.target = cp.lim.ceilingDb;
    smLimTargetLufs.target = cp.lim.targetLufs;
    smLimRelease.target = cp.lim.releaseMs;

    inputMeter.process(buffer);

    processGain(buffer, cp);
    processEq(buffer, cp);
    processComp(buffer, cp);
    processSat(buffer, cp);
    processStereo(buffer, cp);
    processLimiter(buffer, cp);

    if (cp.gain.autoGainMatch)
    {
        const float targetTrim = juce::jlimit(-12.0f, 12.0f, inputMeter.getLufs() - outputMeter.getLufs());
        smoothedAutoGainTrimDb += (targetTrim - smoothedAutoGainTrimDb) * blockSmoothCoeff;
    }
    else
    {
        smoothedAutoGainTrimDb *= (1.0f - blockSmoothCoeff);
    }
    autoGainTrimDb.store(smoothedAutoGainTrimDb);
    buffer.applyGain(juce::Decibels::decibelsToGain(smoothedAutoGainTrimDb));

    outputMeter.process(buffer);
    outputPeakDb.store(bufferPeakDb(buffer));
}

void MasteringChain::processGain(juce::AudioBuffer<float>& buffer, const ChainParameters&)
{
    if (isBypassed(Param::gainBypass))
        return;
    buffer.applyGain(juce::Decibels::decibelsToGain(smInputTrim.tick(blockSmoothCoeff)));
}

void MasteringChain::processEq(juce::AudioBuffer<float>& buffer, const ChainParameters& cp)
{
    if (isBypassed(Param::eqBypass))
        return;

    const float coeff = blockSmoothCoeff;
    const float tilt = smEqTilt.tick(coeff);
    const float lowGain = smEqLowShelfGain.tick(coeff) - tilt * 3.0f;
    const float highGain = smEqHighShelfGain.tick(coeff) + tilt * 3.0f;
    const float lowMidGain = smEqLowMidGain.tick(coeff);
    const float highMidGain = smEqHighMidGain.tick(coeff);

    // Guard against a frequency at or above Nyquist (e.g. a low host sample
    // rate) blowing up the tan() term inside the coefficient formulas and
    // producing NaN/Inf, which would otherwise poison the filter's internal
    // state (and the whole output) until the next reset().
    const double nyquistGuard = sampleRate * 0.49;
    const double lowShelfFreq = juce::jlimit(20.0, nyquistGuard, (double) cp.eq.lowShelfFreq);
    const double lowMidFreq = juce::jlimit(20.0, nyquistGuard, (double) cp.eq.lowMidFreq);
    const double highMidFreq = juce::jlimit(20.0, nyquistGuard, (double) cp.eq.highMidFreq);
    const double highShelfFreq = juce::jlimit(20.0, nyquistGuard, (double) cp.eq.highShelfFreq);
    const float lowMidQ = juce::jlimit(0.1f, 8.0f, cp.eq.lowMidQ);
    const float highMidQ = juce::jlimit(0.1f, 8.0f, cp.eq.highMidQ);

    // ProcessorDuplicator shares ONE Coefficients object between its own
    // .state pointer and every per-channel filter it owns internally,
    // linked once at prepare() time. Reassigning .state here would only
    // repoint OUR pointer to a new object and leave every already-bound
    // per-channel filter processing with stale coefficients forever, so
    // this must mutate the shared object in place instead.
    *eqLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lowShelfFreq, 0.7f, juce::Decibels::decibelsToGain(lowGain));
    *eqLowMid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, lowMidFreq, lowMidQ, juce::Decibels::decibelsToGain(lowMidGain));
    *eqHighMid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, highMidFreq, highMidQ, juce::Decibels::decibelsToGain(highMidGain));
    *eqHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, highShelfFreq, 0.7f, juce::Decibels::decibelsToGain(highGain));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    eqLowShelf.process(context);
    eqLowMid.process(context);
    eqHighMid.process(context);
    eqHighShelf.process(context);
}

void MasteringChain::processComp(juce::AudioBuffer<float>& buffer, const ChainParameters& cp)
{
    if (isBypassed(Param::compBypass))
    {
        compGainReductionDb.store(0.0f);
        return;
    }

    const float coeff = blockSmoothCoeff;
    const float threshold = smCompThreshold.tick(coeff);
    const float baseRatio = smCompRatio.tick(coeff);
    const float baseAttack = smCompAttack.tick(coeff);
    const float baseRelease = smCompRelease.tick(coeff);
    const float makeup = smCompMakeup.tick(coeff);
    const float glue = cp.comp.glue;

    // Glue nudges ratio/attack/release together around the user's base
    // settings rather than overriding them outright.
    const float ratio = baseRatio * (0.7f + 0.6f * glue);
    const float attack = baseAttack * (1.3f - 0.6f * glue);
    const float release = baseRelease * (1.3f - 0.6f * glue);

    compressor.setThreshold(threshold);
    compressor.setRatio(juce::jmax(1.01f, ratio));
    compressor.setAttack(juce::jmax(0.1f, attack));
    compressor.setRelease(juce::jmax(1.0f, release));

    const float preDb = bufferRmsDb(buffer);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);

    // juce::dsp::Compressor doesn't expose its gain reduction directly;
    // approximate it from the RMS delta for the UI meter only.
    compGainReductionDb.store(juce::jmax(0.0f, preDb - bufferRmsDb(buffer)));

    compMakeupGain.setGainDecibels(makeup);
    compMakeupGain.process(context);
}

void MasteringChain::processSat(juce::AudioBuffer<float>& buffer, const ChainParameters& cp)
{
    if (isBypassed(Param::satBypass))
        return;

    const float coeff = blockSmoothCoeff;
    const float drive = smSatDrive.tick(coeff);
    const float mixPct = smSatMix.tick(coeff);
    const float tone = smSatTone.tick(coeff);
    const auto character = (Saturator::Character) cp.sat.character;

    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
        satDryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    const float driveGain = juce::Decibels::decibelsToGain(juce::jmap(drive, 0.0f, 1.0f, 0.0f, 20.0f));
    const float makeupCompensation = 1.0f / std::sqrt(1.0f + drive * 3.0f);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            d[i] = Saturator::shape(character, d[i] * driveGain) * makeupCompensation;
    }

    // Same ProcessorDuplicator sharing rule as processEq() -- see comment there.
    *satToneLow.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, 200.0, 0.7f, juce::Decibels::decibelsToGain(-tone * 4.0f));
    *satToneHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 6000.0, 0.7f, juce::Decibels::decibelsToGain(tone * 4.0f));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    satToneLow.process(context);
    satToneHigh.process(context);

    const float wet = mixPct / 100.0f;
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        const auto* dry = satDryBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            d[i] = dry[i] * (1.0f - wet) + d[i] * wet;
    }
}

void MasteringChain::processStereo(juce::AudioBuffer<float>& buffer, const ChainParameters& cp)
{
    if (isBypassed(Param::stereoBypass))
        return;

    const float coeff = blockSmoothCoeff;
    stereoImager.setCrossovers(cp.stereo.lowCrossoverHz, cp.stereo.highCrossoverHz);
    stereoImager.process(buffer,
        smStereoLowWidth.tick(coeff),
        smStereoMidWidth.tick(coeff),
        smStereoHighWidth.tick(coeff),
        smStereoWidthMacro.tick(coeff));

    if (buffer.getNumChannels() >= 2)
    {
        const auto* l = buffer.getReadPointer(0);
        const auto* r = buffer.getReadPointer(1);
        double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            sumLR += (double) l[i] * r[i];
            sumLL += (double) l[i] * l[i];
            sumRR += (double) r[i] * r[i];
        }
        const double denom = std::sqrt(sumLL * sumRR);
        outputCorrelation.store(denom > 1.0e-9 ? (float) (sumLR / denom) : 1.0f);
    }
}

void MasteringChain::processLimiter(juce::AudioBuffer<float>& buffer, const ChainParameters& cp)
{
    if (isBypassed(Param::limBypass))
    {
        limGainReductionDb.store(0.0f);
        return;
    }

    const float coeff = blockSmoothCoeff;
    const float ceiling = smLimCeiling.tick(coeff);
    const float targetLufs = smLimTargetLufs.tick(coeff);
    const float baseRelease = smLimRelease.tick(coeff);
    const float punch = cp.lim.punchVsLoud; // 0 = punch/transient-preserving, 1 = loud/dense

    // More "punch" -> longer release (transients recover before the next
    // hit); more "loud" -> shorter release (denser, louder average level).
    const float effectiveRelease = juce::jmap(punch, baseRelease * 1.5f, baseRelease * 0.6f);

    const float targetTrimDb = juce::jlimit(-24.0f, 24.0f, targetLufs - inputMeter.getLufs());
    limiterTargetTrim.setGainDecibels(targetTrimDb);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> trimContext(block);
    limiterTargetTrim.process(trimContext);

    const float prePeakDb = bufferPeakDb(buffer);

    limiter.setThreshold(ceiling);
    limiter.setRelease(juce::jmax(1.0f, effectiveRelease));

    auto upBlock = oversampling->processSamplesUp(block);
    juce::dsp::ProcessContextReplacing<float> osContext(upBlock);
    limiter.process(osContext);
    oversampling->processSamplesDown(block);

    limGainReductionDb.store(juce::jmax(0.0f, prePeakDb - bufferPeakDb(buffer)));
}
