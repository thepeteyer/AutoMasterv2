#include "TabbedChainEditor.h"

namespace
{
    // A muted version of each tab's accent colour for the tab bar button
    // itself, so the six stages are colour-coded at a glance without the
    // bar turning into a rainbow.
    juce::Colour tabBarColour(juce::Colour accent)
    {
        return juce::Colour(0xff1c1e22).interpolatedWith(accent, 0.35f);
    }
}

TabbedChainEditor::TabbedChainEditor(AutoMasterAudioProcessor& processor)
    : juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop)
{
    setTabBarDepth(30);
    setOutline(0);

    auto gain = makeGainTab(processor);
    gainTab = gain.get();
    addTab("Gain Staging", tabBarColour(TabColours::gain), gain.release(), true);

    auto eq = makeEqTab(processor);
    addTab("EQ", tabBarColour(TabColours::eq), eq.release(), true);

    auto comp = makeCompTab(processor);
    compTab = comp.get();
    addTab("Compression", tabBarColour(TabColours::comp), comp.release(), true);

    auto sat = makeSatTab(processor);
    addTab("Saturation", tabBarColour(TabColours::sat), sat.release(), true);

    auto stereo = makeStereoTab(processor);
    stereoTab = stereo.get();
    addTab("Stereo", tabBarColour(TabColours::stereo), stereo.release(), true);

    auto limiter = makeLimiterTab(processor);
    limiterTab = limiter.get();
    addTab("Limiter", tabBarColour(TabColours::limiter), limiter.release(), true);
}
