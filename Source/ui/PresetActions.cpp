#include "PresetActions.h"

PresetActions::PresetActions (PresetManager& p, PromptOverlay& o)
    : presets (p), prompt (o)
{
}

void PresetActions::addMenuItems (juce::PopupMenu& menu)
{
    const auto user = currentUserPreset();

    menu.addItem ("Init Preset", [this] { presets.loadInit(); });
    menu.addSeparator();
    menu.addItem ("Save As...", [this] { saveAs(); });
    menu.addItem ("Rename...", user.has_value(), false, [this, user] { rename (*user); });
    menu.addItem ("Delete", user.has_value(), false, [this, user] { remove (*user); });
    menu.addSeparator();
    menu.addItem ("Show Preset Folder", [this] { presets.showUserDirectory(); });
}

//==============================================================================
void PresetActions::save()
{
    // User presets save in place; anything else needs a name first
    if (const auto user = currentUserPreset())
        report (presets.saveUserPreset (user->name, user->category));
    else
        saveAs();
}

std::optional<PresetManager::Preset> PresetActions::currentUserPreset() const
{
    if (auto current = presets.getCurrentPreset(); current.has_value() && ! current->isFactory)
        return current;

    return std::nullopt;
}

void PresetActions::saveAs()
{
    // Factory names are reserved, so don't suggest one
    const auto current = presets.getCurrent();
    const auto suggestion = current.isFactory ? juce::String() : current.name;

    prompt.askForText ("Save Preset", suggestion, "Save", [this] (const juce::String& name)
    {
        if (! presets.userPresetExists (name))
        {
            saveAs (name);
            return;
        }

        prompt.askToConfirm ("Replace Preset", "\"" + name.trim() + "\" already exists. Replace it?", "Replace",
                             [this, name] { saveAs (name); });
    });
}

void PresetActions::saveAs (const juce::String& name)
{
    report (presets.saveUserPreset (name, presets.getCurrent().category));
}

void PresetActions::rename (const PresetManager::Preset& preset)
{
    prompt.askForText ("Rename Preset", preset.name, "Rename", [this, preset] (const juce::String& name)
    {
        report (presets.renameUserPreset (preset, name));
    });
}

void PresetActions::remove (const PresetManager::Preset& preset)
{
    prompt.askToConfirm ("Delete Preset", "Delete \"" + preset.name + "\"?", "Delete",
                         [this, preset] { report (presets.deleteUserPreset (preset)); });
}

void PresetActions::report (const juce::Result& result)
{
    if (result.failed())
        prompt.showMessage ("Preset", result.getErrorMessage());
}
