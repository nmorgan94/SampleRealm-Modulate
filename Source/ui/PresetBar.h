#pragma once

#include "../presets/PresetManager.h"

class PresetActions;

//==============================================================================
/**
 * Preset browsing for a PresetManager, and a Save button.
 */
class PresetBar : public juce::Component,
                  private juce::Timer
{
public:
    PresetBar (PresetManager&, PresetActions&);

    void resized() override;

private:
    PresetManager& presets;
    PresetActions& actions;

    juce::TextButton previousButton { "<" }, nextButton { ">" }, nameButton, saveButton { "Save" };
    juce::uint32 lastChange = 0;

    void updateName();
    void showPresetMenu();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
