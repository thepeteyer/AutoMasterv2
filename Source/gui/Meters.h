#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Compact readout strip: input/output LUFS, output peak, mono-compatibility
// (stereo correlation) and the auto-gain-match trim currently being applied.
// Per-stage gain-reduction numbers are pushed into their own tab's meter
// label instead (see StageTabPanel::setMeterText), so this strip stays global.
class MeterStrip : public juce::Component
{
public:
    explicit MeterStrip(AutoMasterAudioProcessor& processorToUse);

    void update();

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    AutoMasterAudioProcessor& processor;
    juce::Label inLufsLabel, outLufsLabel, peakLabel, correlationLabel, autoGainLabel;

    static void setReading(juce::Label& label, const juce::String& title, const juce::String& value);
};
