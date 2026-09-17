#include "RulesEngine.h"
#include <JuceHeader.h>
#include <numeric>

// Every heuristic below reasons from the same six AnalysisResult features
// (spectral tilt/bands, crest factor, LUFS, correlation, transient density).
// Numbers are deliberately conservative -- this is meant to hand the user a
// sane, gentle starting point to fine-tune, not a finished master.
ChainParameters DefaultRulesEngine::derive(const AnalysisResult& a) const
{
    ChainParameters c;
    if (!a.valid)
        return c; // struct defaults are already a reasonable neutral starting chain

    // --- Gain staging: leave trim at 0, always suggest auto-match on -----
    c.gain.inputTrimDb = 0.0f;
    c.gain.autoGainMatch = true;

    // --- EQ: pull the measured tilt toward a gentle -1 dB/oct reference --
    {
        constexpr float targetTilt = -1.0f;
        const float deviation = a.spectralTiltDbPerOctave - targetTilt; // >0 = brighter than target

        c.eq.highShelfGainDb = juce::jlimit(-4.0f, 4.0f, -deviation * 1.5f);
        c.eq.lowShelfGainDb = juce::jlimit(-3.0f, 3.0f, deviation * 0.6f);

        const float avgBandDb = std::accumulate(a.bandEnergyDb.begin(), a.bandEnergyDb.end(), 0.0f) / 6.0f;
        c.eq.lowMidGainDb = juce::jlimit(-3.0f, 3.0f, -(a.bandEnergyDb[2] - avgBandDb) * 0.3f);
        c.eq.highMidGainDb = juce::jlimit(-3.0f, 3.0f, -(a.bandEnergyDb[4] - avgBandDb) * 0.3f);
        c.eq.tilt = 0.0f;
    }

    // --- Compression: crest factor drives ratio/attack/release/threshold -
    {
        const float t = juce::jlimit(0.0f, 1.0f, (16.0f - a.crestFactorDb) / (16.0f - 8.0f));
        c.comp.ratio = juce::jmap(t, 1.5f, 2.5f);
        c.comp.attackMs = juce::jmap(t, 30.0f, 10.0f) * (1.0f + a.transientDensity * 0.5f);
        c.comp.releaseMs = juce::jmap(t, 220.0f, 100.0f);
        c.comp.thresholdDb = a.rmsDb + juce::jmap(t, 8.0f, 3.0f);
        c.comp.makeupDb = juce::jlimit(0.0f, 6.0f,
            (a.rmsDb - c.comp.thresholdDb) * (1.0f - 1.0f / c.comp.ratio) * 0.5f);
        c.comp.glue = juce::jmap(t, 0.2f, 0.8f);
    }

    // --- Saturation: subtle by default, a touch more on dense material ---
    {
        const bool dense = a.crestFactorDb < 10.0f;
        c.sat.drive = dense ? 0.22f : 0.08f;
        c.sat.mix = dense ? 25.0f : 10.0f;
        c.sat.character = a.stereoCorrelation < 0.5f ? 2 /* Transistor */ : 0 /* Tape */;
        c.sat.toneTilt = juce::jlimit(-0.3f, 0.3f, -a.spectralTiltDbPerOctave * 0.05f);
    }

    // --- Stereo imaging: mono the lows, especially if phase looks shaky ---
    {
        const float corr = juce::jlimit(-1.0f, 1.0f, a.stereoCorrelation);
        c.stereo.lowWidth = corr < 0.5f ? 0.05f : 0.15f;
        c.stereo.lowCrossoverHz = a.bandEnergyDb[0] > a.bandEnergyDb[1] ? 100.0f : 150.0f;
        c.stereo.midWidth = juce::jmap(juce::jlimit(0.0f, 1.0f, corr), 1.0f, 1.15f);
        c.stereo.highWidth = juce::jmap(juce::jlimit(0.0f, 1.0f, corr), 1.05f, 1.2f);
        c.stereo.highCrossoverHz = 4000.0f;
        c.stereo.widthMacro = 1.0f;
    }

    // --- Limiter: streaming-safe by default, louder if source already is -
    {
        c.lim.targetLufs = a.integratedLufs > -9.0f ? -9.0f : -14.0f;
        c.lim.ceilingDb = c.lim.targetLufs > -10.0f ? -0.3f : -1.0f;
        c.lim.releaseMs = juce::jmap(juce::jlimit(4.0f, 20.0f, a.crestFactorDb), 4.0f, 20.0f, 30.0f, 120.0f);
        c.lim.punchVsLoud = juce::jmap(juce::jlimit(4.0f, 20.0f, a.crestFactorDb), 4.0f, 20.0f, 0.3f, 0.75f);
    }

    return c;
}
