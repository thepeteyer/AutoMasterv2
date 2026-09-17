#include "PluginEditor.h"

AutoMasterAudioProcessorEditor::AutoMasterAudioProcessorEditor(AutoMasterAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p), transportBar(p), meterStrip(p), tabs(p)
{
    addAndMakeVisible(transportBar);
    addAndMakeVisible(meterStrip);
    addAndMakeVisible(tabs);

    setSize(820, 600);
    setResizable(true, true);
    setResizeLimits(680, 520, 1400, 950);

    startTimerHz(15);
}

AutoMasterAudioProcessorEditor::~AutoMasterAudioProcessorEditor()
{
    stopTimer();
}

void AutoMasterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101114));
}

void AutoMasterAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();
    transportBar.setBounds(r.removeFromTop(104));
    meterStrip.setBounds(r.removeFromTop(44));
    tabs.setBounds(r);
}

void AutoMasterAudioProcessorEditor::timerCallback()
{
    transportBar.refresh();
    meterStrip.update();

    auto& chain = processor.getChain();
    tabs.getCompTab().setMeterText("GR: " + juce::String(chain.getCompressorGainReductionDb(), 1) + " dB");
    tabs.getStereoTab().setMeterText("Correlation: " + juce::String(chain.getOutputCorrelation(), 2));
    tabs.getLimiterTab().setMeterText("GR: " + juce::String(chain.getLimiterGainReductionDb(), 1)
        + " dB  |  True peak: " + juce::String(chain.getOutputPeakDb(), 1) + " dB");
}
