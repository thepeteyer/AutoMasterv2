#include "StageTabs.h"
#include "../Params.h"

// --- KnobControl ---------------------------------------------------------
KnobControl::KnobControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                          const juce::String& displayName, const juce::String& tooltip, bool isHero)
    : hero(isHero)
{
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, hero ? 92 : 72, hero ? 20 : 18);
    slider.setTooltip(tooltip);

    // Guarantee the unit (dB/Hz/ms/LUFS/...) is always visible in the
    // numeric readout, regardless of how the host chooses to format text.
    if (auto* param = apvts.getParameter(paramId))
    {
        const auto unit = param->getLabel();
        if (unit.isNotEmpty())
            slider.setTextValueSuffix(" " + unit);
    }
    addAndMakeVisible(slider);

    label.setText(displayName, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(hero ? 17.0f : 13.0f, hero ? juce::Font::bold : juce::Font::plain));
    label.setTooltip(tooltip);
    addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
}

void KnobControl::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(hero ? 24 : 18));
    slider.setBounds(r);
}

// --- ToggleControl ---------------------------------------------------------
ToggleControl::ToggleControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                              const juce::String& displayName, const juce::String& tooltip)
{
    button.setButtonText(displayName);
    button.setTooltip(tooltip);
    addAndMakeVisible(button);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, button);
}

void ToggleControl::resized()
{
    button.setBounds(getLocalBounds().withSizeKeepingCentre(getWidth(), 26));
}

// --- ChoiceControl ---------------------------------------------------------
ChoiceControl::ChoiceControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                              const juce::String& displayName, const juce::String& tooltip)
{
    label.setText(displayName, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(13.0f));
    label.setTooltip(tooltip);
    addAndMakeVisible(label);

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId)))
        combo.addItemList(choiceParam->choices, 1);
    combo.setTooltip(tooltip);
    addAndMakeVisible(combo);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramId, combo);
}

void ChoiceControl::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(18));
    combo.setBounds(r.withSizeKeepingCentre(r.getWidth(), 24));
}

// --- StageTabPanel -----------------------------------------------------
StageTabPanel::StageTabPanel(AutoMasterAudioProcessor& processorToUse, int stageIndexToUse,
                              const char* bypassParamId, juce::Colour accentColourToUse)
    : processor(processorToUse), stageIndex(stageIndexToUse), accentColour(accentColourToUse)
{
    bypassButton.setTooltip("Skip this stage entirely -- the audio passes straight through untouched.");
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, bypassParamId, bypassButton);

    resetButton.setTooltip("Throw away your tweaks on this tab and go back to what the AI suggested from Learn mode.");
    resetButton.onClick = [this] { processor.resetStageToSuggestion(stageIndex); };
    addAndMakeVisible(resetButton);

    heroCaption.setJustificationType(juce::Justification::centredTop);
    heroCaption.setFont(juce::Font(11.5f));
    heroCaption.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.65f));
    addAndMakeVisible(heroCaption);

    advancedToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff26282e));
    advancedToggle.setTooltip("Show or hide the detailed, engineer-level controls for this stage.");
    advancedToggle.onClick = [this]
    {
        advancedExpanded = !advancedExpanded;
        updateAdvancedVisibility();
        resized();
    };
    addAndMakeVisible(advancedToggle);

    addChildComponent(advancedPanel); // hidden until the user expands it
    updateAdvancedVisibility();

    meterLabel.setJustificationType(juce::Justification::centredRight);
    meterLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(meterLabel);
}

void StageTabPanel::setHero(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip)
{
    heroKnob = std::make_unique<KnobControl>(processor.apvts, paramId, displayName, tooltip, true);
    addAndMakeVisible(*heroKnob);
    heroCaption.setText(tooltip, juce::dontSendNotification);
}

void StageTabPanel::addAdvancedKnob(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip)
{
    advancedControls.push_back(std::make_unique<KnobControl>(processor.apvts, paramId, displayName, tooltip, false));
    advancedPanel.addAndMakeVisible(*advancedControls.back());
}

void StageTabPanel::addAdvancedToggle(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip)
{
    advancedControls.push_back(std::make_unique<ToggleControl>(processor.apvts, paramId, displayName, tooltip));
    advancedPanel.addAndMakeVisible(*advancedControls.back());
}

void StageTabPanel::addAdvancedChoice(const juce::String& paramId, const juce::String& displayName, const juce::String& tooltip)
{
    advancedControls.push_back(std::make_unique<ChoiceControl>(processor.apvts, paramId, displayName, tooltip));
    advancedPanel.addAndMakeVisible(*advancedControls.back());
}

void StageTabPanel::updateAdvancedVisibility()
{
    advancedToggle.setButtonText(advancedExpanded ? "Advanced \xE2\x96\xBE" : "Advanced \xE2\x96\xB8");
    advancedPanel.setVisible(advancedExpanded);
}

void StageTabPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1c1e22));
    g.setColour(accentColour);
    g.fillRect(getLocalBounds().removeFromTop(3));
}

void StageTabPanel::resized()
{
    auto r = getLocalBounds().reduced(10);

    // Header: bypass and reset sit in the same corner on every tab.
    auto header = r.removeFromTop(30);
    bypassButton.setBounds(header.removeFromLeft(90));
    resetButton.setBounds(header.removeFromRight(190));

    auto footer = r.removeFromBottom(20);
    meterLabel.setBounds(footer);

    auto advancedToggleRow = r.removeFromBottom(28);
    advancedToggle.setBounds(advancedToggleRow.removeFromLeft(150));

    if (advancedExpanded)
    {
        auto advArea = r.removeFromBottom(120);
        advancedPanel.setBounds(advArea);

        const int n = (int) advancedControls.size();
        if (n > 0)
        {
            auto inner = advancedPanel.getLocalBounds();
            const int w = juce::jmax(70, inner.getWidth() / n);
            for (int i = 0; i < n; ++i)
                advancedControls[(size_t) i]->setBounds(inner.removeFromLeft(w).reduced(6));
        }
    }

    // Whatever's left is the hero area -- the one knob a beginner needs.
    if (heroKnob != nullptr)
    {
        auto heroArea = r.reduced(20);
        auto captionArea = heroArea.removeFromBottom(42);
        const int heroSize = juce::jlimit(80, 170, juce::jmin(heroArea.getWidth(), heroArea.getHeight()));
        auto heroBounds = juce::Rectangle<int>(0, 0, heroSize, heroSize).withCentre(heroArea.getCentre());
        heroKnob->setBounds(heroBounds);
        heroCaption.setBounds(captionArea);
    }
}

// --- Concrete tabs -------------------------------------------------------
// Every tab follows the same shape: one large "hero" macro knob a beginner
// can use on its own, plus the detailed engineer knobs tucked behind
// "Advanced" for anyone who wants to go deeper.
std::unique_ptr<StageTabPanel> makeGainTab(AutoMasterAudioProcessor& p)
{
    auto tab = std::make_unique<StageTabPanel>(p, 0, Param::gainBypass, TabColours::gain);
    tab->setHero(Param::gainInputTrim, "Input Trim",
        "Turns the incoming signal up or down before anything else in the chain touches it. Use this to get a healthy starting level.");
    tab->addAdvancedToggle(Param::gainAutoMatch, "Auto Gain Match",
        "Automatically evens out the volume between bypassed and processed audio, so A/B comparisons are fair instead of \"louder just sounds better\".");
    return tab;
}

std::unique_ptr<StageTabPanel> makeEqTab(AutoMasterAudioProcessor& p)
{
    auto tab = std::make_unique<StageTabPanel>(p, 1, Param::eqBypass, TabColours::eq);
    tab->setHero(Param::eqTilt, "Tilt",
        "One knob for overall brightness. Turn right for a brighter mix, left for a warmer, darker one.");
    tab->addAdvancedKnob(Param::eqLowShelfFreq, "Low Freq", "Where the low-end boost or cut starts.");
    tab->addAdvancedKnob(Param::eqLowShelfGain, "Low Gain", "How much bass to add or remove.");
    tab->addAdvancedKnob(Param::eqLowMidFreq, "Low-Mid Freq", "Targets boxy or muddy low-mid frequencies.");
    tab->addAdvancedKnob(Param::eqLowMidGain, "Low-Mid Gain", "How much to boost or cut the low-mids.");
    tab->addAdvancedKnob(Param::eqHighMidFreq, "High-Mid Freq", "Targets presence and clarity in the upper mids.");
    tab->addAdvancedKnob(Param::eqHighMidGain, "High-Mid Gain", "How much to boost or cut the upper mids.");
    tab->addAdvancedKnob(Param::eqHighShelfFreq, "High Freq", "Where the treble boost or cut starts.");
    tab->addAdvancedKnob(Param::eqHighShelfGain, "High Gain", "How much air or sparkle to add or remove.");
    return tab;
}

std::unique_ptr<StageTabPanel> makeCompTab(AutoMasterAudioProcessor& p)
{
    auto tab = std::make_unique<StageTabPanel>(p, 2, Param::compBypass, TabColours::comp);
    tab->setHero(Param::compGlue, "Glue",
        "One knob for overall cohesion -- turn it up to make the mix feel tighter and more \"glued together\".");
    tab->addAdvancedKnob(Param::compThreshold, "Threshold", "The level above which compression starts working.");
    tab->addAdvancedKnob(Param::compRatio, "Ratio", "How strongly the compressor squashes the signal once it's above the threshold.");
    tab->addAdvancedKnob(Param::compAttack, "Attack", "How quickly compression kicks in after a loud moment.");
    tab->addAdvancedKnob(Param::compRelease, "Release", "How quickly compression lets go after the loud moment passes.");
    tab->addAdvancedKnob(Param::compMakeup, "Makeup", "Turns the level back up afterwards to compensate for the reduction.");
    return tab;
}

std::unique_ptr<StageTabPanel> makeSatTab(AutoMasterAudioProcessor& p)
{
    auto tab = std::make_unique<StageTabPanel>(p, 3, Param::satBypass, TabColours::sat);
    tab->setHero(Param::satDrive, "Drive",
        "How much warmth and harmonic character to add. Turn up for more grit and richness.");
    tab->addAdvancedChoice(Param::satCharacter, "Character", "The flavour of saturation: Tape, Tube, Transistor, or Clip.");
    tab->addAdvancedKnob(Param::satMix, "Mix", "Blends the saturated signal back in with the clean original.");
    tab->addAdvancedKnob(Param::satTone, "Tone Tilt", "Shapes the tone of the added saturation -- darker or brighter.");
    return tab;
}

std::unique_ptr<StageTabPanel> makeStereoTab(AutoMasterAudioProcessor& p)
{
    auto tab = std::make_unique<StageTabPanel>(p, 4, Param::stereoBypass, TabColours::stereo);
    tab->setHero(Param::stereoWidthMacro, "Width",
        "One knob to widen or narrow the overall stereo image.");
    tab->addAdvancedKnob(Param::stereoLowWidth, "Low Width",
        "How wide the bass is allowed to be -- usually kept narrow (near-mono) for a solid, speaker-friendly low end.");
    tab->addAdvancedKnob(Param::stereoLowXover, "Low Xover", "The frequency below which the Low Width setting applies.");
    tab->addAdvancedKnob(Param::stereoMidWidth, "Mid Width", "How wide the mid-range -- vocals, instruments -- sounds.");
    tab->addAdvancedKnob(Param::stereoHighWidth, "High Width", "How wide the top end -- air, cymbals -- sounds.");
    tab->addAdvancedKnob(Param::stereoHighXover, "High Xover", "The frequency above which the High Width setting applies.");
    return tab;
}

std::unique_ptr<StageTabPanel> makeLimiterTab(AutoMasterAudioProcessor& p)
{
    auto tab = std::make_unique<StageTabPanel>(p, 5, Param::limBypass, TabColours::limiter);
    tab->setHero(Param::limPunchVsLoud, "Punch vs Loud",
        "Balances preserving punch and transients against squeezing out maximum loudness.");
    tab->addAdvancedKnob(Param::limCeiling, "Ceiling", "The absolute maximum output level allowed, so nothing clips downstream.");
    tab->addAdvancedKnob(Param::limTargetLufs, "Target Loudness", "The overall loudness the plugin aims for, measured in LUFS.");
    tab->addAdvancedKnob(Param::limRelease, "Release", "How quickly the limiter recovers after reining in a peak.");
    return tab;
}
