#include "PresetBar.h"
#include "PresetActions.h"

PresetBar::PresetBar (PresetManager& p, PresetActions& a)
    : presets (p), actions (a)
{
    previousButton.onClick = [this] { presets.loadPrevious(); };
    nextButton.onClick     = [this] { presets.loadNext(); };
    nameButton.onClick     = [this] { showPresetMenu(); };
    saveButton.onClick     = [this] { actions.save(); };

    for (auto* b : { &previousButton, &nextButton, &nameButton, &saveButton })
        addAndMakeVisible (b);

    updateName();
    startTimerHz (10);
}

void PresetBar::resized()
{
    auto bounds = getLocalBounds();

    saveButton.setBounds (bounds.removeFromRight (48));
    bounds.removeFromRight (8);

    previousButton.setBounds (bounds.removeFromLeft (24));
    nextButton.setBounds (bounds.removeFromRight (24));
    nameButton.setBounds (bounds.reduced (4, 0));
}

void PresetBar::timerCallback()
{
    if (presets.getChangeCounter() != lastChange)
        updateName();
}

void PresetBar::updateName()
{
    lastChange = presets.getChangeCounter();
    nameButton.setButtonText (presets.getCurrent().name + (presets.isModified() ? " *" : ""));
}

//==============================================================================
void PresetBar::showPresetMenu()
{
    presets.refreshUserPresets();

    auto presetList = presets.getPresets();
    const auto current = presets.getCurrent();

    const auto makeItem = [&] (int index)
    {
        juce::PopupMenu::Item item (presetList.getReference (index).name);
        item.itemID = index + 1;
        item.isTicked = current.matches (presetList.getReference (index));
        return item;
    };

    // Factory presets by category, user presets flat
    std::map<juce::String, juce::PopupMenu> categories;
    juce::String currentCategory;
    juce::Array<int> userIndices;

    for (int i = 0; i < presetList.size(); ++i)
    {
        const auto& preset = presetList.getReference (i);

        if (! preset.isFactory)
        {
            userIndices.add (i);
            continue;
        }

        const auto category = preset.category.isNotEmpty() ? preset.category : juce::String ("Other");
        categories[category].addItem (makeItem (i));

        if (current.matches (preset))
            currentCategory = category;
    }

    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    menu.addSectionHeader ("Factory");

    for (auto& [category, subMenu] : categories)
        menu.addSubMenu (category, subMenu, true, nullptr, category == currentCategory);

    if (categories.empty())
        menu.addItem ("No factory presets yet", false, false, nullptr);

    menu.addSectionHeader ("User");

    for (auto i : userIndices)
        menu.addItem (makeItem (i));

    if (userIndices.isEmpty())
        menu.addItem ("No user presets yet", false, false, nullptr);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (nameButton),
                        [safeThis = juce::Component::SafePointer (this), listed = std::move (presetList)] (int result)
                        {
                            if (safeThis != nullptr && result > 0)
                                safeThis->presets.loadPreset (listed.getReference (result - 1));
                        });
}
