#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Top transport strip -- the primary workflow control. Learn / Bypass /
// Apply / Freeze mode switch (icon + label, active mode unmistakable),
// the Continuous/Adaptive toggle + sensitivity knob, and a status line.
class TransportBar : public juce::Component
{
public:
    explicit TransportBar(AutoMasterAudioProcessor& processorToUse);

    void refresh(); // called by the editor's timer to reflect current mode / analysis progress

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void setMode(TransportMode mode);
    void updateButtonStates();

    AutoMasterAudioProcessor& processor;

    juce::Label modeHeading; // large "Current: Apply" style readout

    juce::TextButton learnButton   { "\xE2\x96\xB6  Learn" };
    juce::TextButton bypassButton  { "\xE2\xAC\x9B  Bypass" };
    juce::TextButton applyButton   { "\xE2\x9C\x93  Apply" };
    juce::TextButton freezeButton  { "\xE2\x9D\x84  Freeze" };

    juce::ToggleButton adaptiveToggle { "Continuous / Adaptive" };
    juce::Slider sensitivitySlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label sensitivityLabel { {}, "Sensitivity" };

    juce::Label statusLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> adaptiveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensitivityAttachment;
};
