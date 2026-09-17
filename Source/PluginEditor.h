#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/TransportBar.h"
#include "gui/Meters.h"
#include "gui/TabbedChainEditor.h"

class AutoMasterAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AutoMasterAudioProcessorEditor(AutoMasterAudioProcessor&);
    ~AutoMasterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    AutoMasterAudioProcessor& processor;

    juce::TooltipWindow tooltipWindow { this, 500 };

    TransportBar transportBar;
    MeterStrip meterStrip;
    TabbedChainEditor tabs;
};
