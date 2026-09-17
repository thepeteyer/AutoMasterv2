#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Small self-contained controls, each wired straight to an APVTS parameter.
// All three take a plain-language `tooltip` shown on hover -- the audience
// is musicians, not engineers, so tooltips avoid DSP jargon where possible.
class KnobControl : public juce::Component
{
public:
    KnobControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                const juce::String& displayName, const juce::String& tooltip, bool isHero = false);
    void resized() override;

private:
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    bool hero;
};

class ToggleControl : public juce::Component
{
public:
    ToggleControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                  const juce::String& displayName, const juce::String& tooltip);
    void resized() override;

private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

class ChoiceControl : public juce::Component
{
public:
    ChoiceControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                  const juce::String& displayName, const juce::String& tooltip);
    void resized() override;

private:
    juce::Label label;
    juce::ComboBox combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// One tab: a bypass toggle + "Reset to AI suggestion" button in the header
// (same corner on every tab), one large "hero" macro knob a beginner can
// use on its own, and a collapsible Advanced section holding the detailed
// knobs underneath it. An optional bottom meter/info line rounds it out.
class StageTabPanel : public juce::Component
{
public:
    StageTabPanel(AutoMasterAudioProcessor& processorToUse, int stageIndex,
                  const char* bypassParamId, juce::Colour accentColourToUse);

    // The one prominent, beginner-facing control for this tab.
    void setHero(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip);

    // Detail controls, tucked behind the "Advanced" disclosure.
    void addAdvancedKnob(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip);
    void addAdvancedToggle(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip);
    void addAdvancedChoice(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip);

    void setMeterText(const juce::String& text) { meterLabel.setText(text, juce::dontSendNotification); }

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void updateAdvancedVisibility();

    AutoMasterAudioProcessor& processor;
    int stageIndex;
    juce::Colour accentColour;

    juce::ToggleButton bypassButton { "Bypass" };
    juce::TextButton resetButton { "Reset to AI Suggestion" };
    juce::Label meterLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    std::unique_ptr<KnobControl> heroKnob;
    juce::Label heroCaption;

    juce::TextButton advancedToggle { "Advanced \xE2\x96\xB8" }; // U+25B8 small right-pointing triangle
    juce::Component advancedPanel;
    std::vector<std::unique_ptr<juce::Component>> advancedControls;
    bool advancedExpanded = false;
};

std::unique_ptr<StageTabPanel> makeGainTab(AutoMasterAudioProcessor&);
std::unique_ptr<StageTabPanel> makeEqTab(AutoMasterAudioProcessor&);
std::unique_ptr<StageTabPanel> makeCompTab(AutoMasterAudioProcessor&);
std::unique_ptr<StageTabPanel> makeSatTab(AutoMasterAudioProcessor&);
std::unique_ptr<StageTabPanel> makeStereoTab(AutoMasterAudioProcessor&);
std::unique_ptr<StageTabPanel> makeLimiterTab(AutoMasterAudioProcessor&);

// Shared accent colours so the tab bar and each tab's hero knob agree.
namespace TabColours
{
    const juce::Colour gain    { 0xff3fa9f5 };
    const juce::Colour eq      { 0xff35c9a5 };
    const juce::Colour comp    { 0xfff5a623 };
    const juce::Colour sat     { 0xffe8543f };
    const juce::Colour stereo  { 0xffa06cf0 };
    const juce::Colour limiter { 0xff5fd463 };
}
