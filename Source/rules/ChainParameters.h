#pragma once

// A full suggested chain, one struct per tab, mirroring the APVTS parameter
// groups in Params.h. This is the interface boundary between analysis and
// DSP: RulesEngine::derive(AnalysisResult) -> ChainParameters is the "brain",
// and is designed to be swappable for a trained model later (see
// rules/RulesEngine.h) without touching the DSP or UI.
struct GainChainParams
{
    float inputTrimDb = 0.0f;
    bool autoGainMatch = true;
};

struct EqChainParams
{
    float lowShelfFreq = 80.0f, lowShelfGainDb = 0.0f;
    float lowMidFreq = 300.0f, lowMidGainDb = 0.0f, lowMidQ = 0.7f;
    float highMidFreq = 3000.0f, highMidGainDb = 0.0f, highMidQ = 0.7f;
    float highShelfFreq = 9000.0f, highShelfGainDb = 0.0f;
    float tilt = 0.0f;
};

struct CompChainParams
{
    float thresholdDb = -18.0f, ratio = 2.0f, attackMs = 20.0f, releaseMs = 150.0f, makeupDb = 0.0f, glue = 0.3f;
};

struct SatChainParams
{
    float drive = 0.15f;
    int character = 0; // 0=Tape 1=Tube 2=Transistor 3=Clip
    float mix = 20.0f;
    float toneTilt = 0.0f;
};

struct StereoChainParams
{
    float lowWidth = 0.15f, lowCrossoverHz = 120.0f;
    float midWidth = 1.05f, highWidth = 1.1f, highCrossoverHz = 4000.0f;
    float widthMacro = 1.0f;
};

struct LimChainParams
{
    float ceilingDb = -1.0f, targetLufs = -14.0f, releaseMs = 50.0f, punchVsLoud = 0.5f;
};

struct ChainParameters
{
    GainChainParams gain;
    EqChainParams eq;
    CompChainParams comp;
    SatChainParams sat;
    StereoChainParams stereo;
    LimChainParams lim;
};
