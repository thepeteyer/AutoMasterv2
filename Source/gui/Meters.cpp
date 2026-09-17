#include "Meters.h"

MeterStrip::MeterStrip(AutoMasterAudioProcessor& processorToUse) : processor(processorToUse)
{
    for (auto* l : { &inLufsLabel, &outLufsLabel, &peakLabel, &correlationLabel, &autoGainLabel })
    {
        l->setJustificationType(juce::Justification::centred);
        l->setFont(juce::Font(12.0f));
        addAndMakeVisible(*l);
    }
}

void MeterStrip::setReading(juce::Label& label, const juce::String& title, const juce::String& value)
{
    label.setText(title + "\n" + value, juce::dontSendNotification);
}

void MeterStrip::update()
{
    auto& chain = processor.getChain();

    setReading(inLufsLabel, "In LUFS", juce::String(chain.getInputLufs(), 1));
    setReading(outLufsLabel, "Out LUFS", juce::String(chain.getOutputLufs(), 1));
    setReading(peakLabel, "Out Peak", juce::String(chain.getOutputPeakDb(), 1) + " dB");
    setReading(correlationLabel, "Correlation", juce::String(chain.getOutputCorrelation(), 2));
    setReading(autoGainLabel, "Auto Trim", juce::String(chain.getAutoGainMatchTrimDb(), 1) + " dB");
}

void MeterStrip::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1c1e22));
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawRect(getLocalBounds());
}

void MeterStrip::resized()
{
    auto r = getLocalBounds();
    const int w = r.getWidth() / 5;
    for (auto* l : { &inLufsLabel, &outLufsLabel, &peakLabel, &correlationLabel, &autoGainLabel })
        l->setBounds(r.removeFromLeft(w));
}
