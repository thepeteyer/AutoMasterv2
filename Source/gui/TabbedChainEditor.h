#pragma once
#include <JuceHeader.h>
#include "StageTabs.h"

// The 6-tab shell: Gain -> EQ -> Compression -> Saturation -> Stereo -> Limiter.
class TabbedChainEditor : public juce::TabbedComponent
{
public:
    explicit TabbedChainEditor(AutoMasterAudioProcessor& processor);

    StageTabPanel& getGainTab() { return *gainTab; }
    StageTabPanel& getCompTab() { return *compTab; }
    StageTabPanel& getStereoTab() { return *stereoTab; }
    StageTabPanel& getLimiterTab() { return *limiterTab; }

private:
    StageTabPanel* gainTab = nullptr;
    StageTabPanel* compTab = nullptr;
    StageTabPanel* stereoTab = nullptr;
    StageTabPanel* limiterTab = nullptr;
};
