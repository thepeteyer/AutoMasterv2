#include "TransportBar.h"
#include "../Params.h"

namespace
{
    constexpr auto activeColour = 0xff3fa9f5;
    constexpr auto inactiveColour = 0xff2a2d33;
}

TransportBar::TransportBar(AutoMasterAudioProcessor& processorToUse) : processor(processorToUse)
{
    modeHeading.setJustificationType(juce::Justification::centredLeft);
    modeHeading.setFont(juce::Font(15.0f, juce::Font::bold));
    addAndMakeVisible(modeHeading);

    learnButton.setTooltip("Listens to your audio without changing it and analyses it. Play a representative part of the song while this is on.");
    bypassButton.setTooltip("Turns everything off so you hear the untouched original -- use this to A/B against the processed sound.");
    applyButton.setTooltip("Turns the mastering chain on, using the AI's suggestion from Learn (or whatever you've tweaked since).");
    freezeButton.setTooltip("Locks the current settings so nothing -- not automation, not an accidental knob bump -- can change the sound.");

    for (auto* b : { &learnButton, &bypassButton, &applyButton, &freezeButton })
    {
        b->setClickingTogglesState(false);
        b->setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
        addAndMakeVisible(*b);
    }

    learnButton.onClick = [this] { setMode(TransportMode::Learn); };
    bypassButton.onClick = [this] { setMode(TransportMode::Bypass); };
    applyButton.onClick = [this] { setMode(TransportMode::Apply); };
    freezeButton.onClick = [this] { setMode(TransportMode::Freeze); };

    adaptiveToggle.setTooltip("Keeps listening in the background and gently nudges settings over time, instead of a single one-time analysis.");
    addAndMakeVisible(adaptiveToggle);
    adaptiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, Param::continuousAdaptive, adaptiveToggle);

    sensitivityLabel.setJustificationType(juce::Justification::centredRight);
    sensitivityLabel.setTooltip("How strongly Adaptive mode reacts to changes in the music -- higher moves settings further per nudge.");
    addAndMakeVisible(sensitivityLabel);
    sensitivitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    sensitivitySlider.setTooltip(sensitivityLabel.getTooltip());
    addAndMakeVisible(sensitivitySlider);
    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, Param::adaptiveSensitivity, sensitivitySlider);

    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(13.0f, juce::Font::italic));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    addAndMakeVisible(statusLabel);

    updateButtonStates();
}

void TransportBar::setMode(TransportMode mode)
{
    processor.setTransportMode(mode);
    updateButtonStates();
}

void TransportBar::updateButtonStates()
{
    const auto mode = processor.getTransportMode();

    auto style = [mode](juce::TextButton& b, TransportMode m)
    {
        const bool active = mode == m;
        b.setColour(juce::TextButton::buttonColourId, juce::Colour(active ? activeColour : inactiveColour));
    };
    style(learnButton, TransportMode::Learn);
    style(bypassButton, TransportMode::Bypass);
    style(applyButton, TransportMode::Apply);
    style(freezeButton, TransportMode::Freeze);

    const char* modeName = mode == TransportMode::Learn ? "Learn -- listening and analysing"
        : mode == TransportMode::Bypass ? "Bypass -- dry, untouched"
        : mode == TransportMode::Freeze ? "Freeze -- locked, won't change"
        : "Apply -- mastering chain active";
    modeHeading.setText(juce::String("Current mode: ") + modeName, juce::dontSendNotification);
}

void TransportBar::refresh()
{
    updateButtonStates();

    const auto mode = processor.getTransportMode();
    juce::String status;

    if (processor.isAnalysisInProgress())
        status = "Analysing...";
    else
    {
        switch (mode)
        {
            case TransportMode::Learn:   status = "Play representative audio, then press Apply when ready."; break;
            case TransportMode::Bypass:  status = "Bypassed -- listening through, dry."; break;
            case TransportMode::Apply:   status = "Chain active."; break;
            case TransportMode::Freeze:  status = "Frozen -- knob and automation changes won't affect the sound."; break;
        }

        const auto& a = processor.getLastAnalysisResult();
        if (a.valid)
            status << juce::String::formatted("   |   last analysis: %.1f LUFS, %.1f dB crest, %.1fs",
                a.integratedLufs, a.crestFactorDb, a.secondsAnalyzed);
    }

    statusLabel.setText(status, juce::dontSendNotification);
}

void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff15161a));
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawLine(0.0f, (float) getHeight() - 1.0f, (float) getWidth(), (float) getHeight() - 1.0f, 1.5f);
}

void TransportBar::resized()
{
    auto r = getLocalBounds().reduced(8);

    modeHeading.setBounds(r.removeFromTop(20));
    r.removeFromTop(4);

    auto buttons = r.removeFromTop(36);
    const int bw = 110;
    learnButton.setBounds(buttons.removeFromLeft(bw));
    buttons.removeFromLeft(4);
    bypassButton.setBounds(buttons.removeFromLeft(bw));
    buttons.removeFromLeft(4);
    applyButton.setBounds(buttons.removeFromLeft(bw));
    buttons.removeFromLeft(4);
    freezeButton.setBounds(buttons.removeFromLeft(bw));

    buttons.removeFromLeft(20);
    auto adaptiveArea = buttons;
    adaptiveToggle.setBounds(adaptiveArea.removeFromLeft(180));
    sensitivityLabel.setBounds(adaptiveArea.removeFromLeft(70));
    sensitivitySlider.setBounds(adaptiveArea);

    r.removeFromTop(4);
    statusLabel.setBounds(r.removeFromTop(20));
}
