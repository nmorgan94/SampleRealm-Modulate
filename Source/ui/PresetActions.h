#pragma once

#include "../presets/PresetManager.h"
#include "PromptOverlay.h"

//==============================================================================
/**
 * Saving, renaming and deleting presets, with the prompts they need.
 */
class PresetActions
{
public:
    PresetActions (PresetManager&, PromptOverlay&);

    void save();
    void addMenuItems (juce::PopupMenu&);

private:
    PresetManager& presets;
    PromptOverlay& prompt;

    void saveAs();
    void saveAs (const juce::String& name);
    void rename (const PresetManager::Preset&);
    void remove (const PresetManager::Preset&);
    std::optional<PresetManager::Preset> currentUserPreset() const;
    void report (const juce::Result&);

    JUCE_DECLARE_NON_COPYABLE (PresetActions)
};
