#pragma once
#include <JuceHeader.h>

// Central registry of every automatable parameter ID, grouped by chain stage.
// ChainParameters (rules/ChainParameters.h) mirrors this grouping; the AI
// "Reset to suggestion" action writes RulesEngine output straight into these.
namespace Param
{
    constexpr auto continuousAdaptive   = "continuousAdaptive";
    constexpr auto adaptiveSensitivity  = "adaptiveSensitivity";

    constexpr auto gainBypass       = "gain_bypass";
    constexpr auto gainInputTrim    = "gain_inputTrimDb";
    constexpr auto gainAutoMatch    = "gain_autoGainMatch";

    constexpr auto eqBypass         = "eq_bypass";
    constexpr auto eqLowShelfFreq   = "eq_lowShelfFreq";
    constexpr auto eqLowShelfGain   = "eq_lowShelfGainDb";
    constexpr auto eqLowMidFreq     = "eq_lowMidFreq";
    constexpr auto eqLowMidGain     = "eq_lowMidGainDb";
    constexpr auto eqLowMidQ        = "eq_lowMidQ";
    constexpr auto eqHighMidFreq    = "eq_highMidFreq";
    constexpr auto eqHighMidGain    = "eq_highMidGainDb";
    constexpr auto eqHighMidQ       = "eq_highMidQ";
    constexpr auto eqHighShelfFreq  = "eq_highShelfFreq";
    constexpr auto eqHighShelfGain  = "eq_highShelfGainDb";
    constexpr auto eqTilt           = "eq_tilt";

    constexpr auto compBypass       = "comp_bypass";
    constexpr auto compThreshold    = "comp_thresholdDb";
    constexpr auto compRatio        = "comp_ratio";
    constexpr auto compAttack       = "comp_attackMs";
    constexpr auto compRelease      = "comp_releaseMs";
    constexpr auto compMakeup       = "comp_makeupDb";
    constexpr auto compGlue         = "comp_glue";

    constexpr auto satBypass        = "sat_bypass";
    constexpr auto satDrive         = "sat_drive";
    constexpr auto satCharacter     = "sat_character";
    constexpr auto satMix           = "sat_mix";
    constexpr auto satTone          = "sat_toneTilt";

    constexpr auto stereoBypass     = "stereo_bypass";
    constexpr auto stereoLowWidth   = "stereo_lowWidth";
    constexpr auto stereoLowXover   = "stereo_lowCrossoverHz";
    constexpr auto stereoMidWidth   = "stereo_midWidth";
    constexpr auto stereoHighWidth  = "stereo_highWidth";
    constexpr auto stereoHighXover  = "stereo_highCrossoverHz";
    constexpr auto stereoWidthMacro = "stereo_widthMacro";

    constexpr auto limBypass        = "lim_bypass";
    constexpr auto limCeiling       = "lim_ceilingDb";
    constexpr auto limTargetLufs    = "lim_targetLufs";
    constexpr auto limRelease       = "lim_releaseMs";
    constexpr auto limPunchVsLoud   = "lim_punchVsLoud";

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
