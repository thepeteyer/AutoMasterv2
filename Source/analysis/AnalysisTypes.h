#pragma once
#include <array>

// One completed Learn-mode analysis pass. RulesEngine::derive() turns this
// into a ChainParameters suggestion; the UI also displays it directly
// (peak/RMS/LUFS readouts, correlation meter, etc).
struct AnalysisResult
{
    bool valid = false;
    double secondsAnalyzed = 0.0;

    float peakDb = -60.0f;
    float rmsDb = -60.0f;
    float crestFactorDb = 10.0f;

    float integratedLufs = -23.0f;

    // -1 (out of phase) .. 0 (uncorrelated) .. 1 (mono-identical)
    float stereoCorrelation = 1.0f;

    // Average band energy in dB: sub(20-60) bass(60-150) lowMid(150-500)
    // mid(500-2k) highMid(2k-6k) air(6k-20k). Used for tonal-tilt heuristics.
    std::array<float, 6> bandEnergyDb { -60.f, -60.f, -60.f, -60.f, -60.f, -60.f };

    // Linear-regression slope of bandEnergyDb across log-frequency.
    // Negative = darker than flat, positive = brighter than flat.
    float spectralTiltDbPerOctave = 0.0f;

    // Normalised 0..1 onset rate, used as a transient-density proxy for
    // compressor attack/release heuristics.
    float transientDensity = 0.5f;
};
