#pragma once

#include "MasterMeter.h"
#include "PresetActions.h"
#include "PresetBar.h"

//==============================================================================
/**
 * The strip along the top of the editor: the title, the preset bar, the menu and the master gain and meter.
 */
class HeaderBar : public juce::Component
{
public:
    static constexpr int height = 70;

    HeaderBar (const juce::String& title, PresetManager&, PromptOverlay&, std::function<void()> showManual,
               const juce::String& aboutText, PeakMeter& meter, juce::AudioProcessorValueTreeState&);

    void resized() override;

private:
    PromptOverlay& prompt;
    const std::function<void()> showManual;
    const juce::String aboutText;

    juce::Label title, version;
    PresetActions presetActions;
    PresetBar presetBar;
    juce::TextButton menuButton { "..." };
    MasterMeter meter;

    void showMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
