#include "HeaderBar.h"
#include "CustomLookAndFeel.h"

namespace
{
    constexpr float minimumTitleScale = 0.7f;
}

HeaderBar::HeaderBar (const juce::String& titleText, const Layout& l, PresetManager& presets, PromptOverlay& o,
                      std::function<void()> m, const juce::String& about, juce::Component& meterToShow)
    : title (titleText), layout (l), prompt (o), showManual (std::move (m)), aboutText (about),
      presetActions (presets, prompt), presetBar (presets, presetActions), meter (meterToShow)
{
    // For screen readers, as the title is painted
    setTitle (title);

    menuButton.onClick = [this] { showMenu(); };

    addAndMakeVisible (presetBar);
    addAndMakeVisible (menuButton);
    addAndMakeVisible (meter);
}

void HeaderBar::paint (juce::Graphics& g)
{
    g.fillAll (findColour (backgroundColourId));

    // The meter's repaints reach here too
    const auto titleArea = getLocalBounds().withTrimmedLeft (layout.edge).withWidth (layout.titleWidth);
    if (! g.clipRegionIntersects (titleArea))
        return;

    // Painted, so the colour comes from the LookAndFeel
    g.setColour (findColour (titleColourId));
    g.setFont (juce::Font (CustomLookAndFeel::orbitron (CustomLookAndFeel::boldWeight).withHeight (layout.titleHeight)));
    g.drawFittedText (title, titleArea, juce::Justification::centredLeft, 1, minimumTitleScale);
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced (layout.edge, 0);
    meter.setBounds (bounds.removeFromRight (layout.meterWidth).reduced (0, layout.meterInset));

    auto controls = bounds.withTrimmedLeft (layout.titleWidth + layout.gap).withTrimmedRight (layout.gap);
    controls = controls.withSizeKeepingCentre (controls.getWidth(), layout.controlsHeight);
    menuButton.setBounds (controls.removeFromRight (28));
    controls.removeFromRight (4);
    presetBar.setBounds (controls);
}

void HeaderBar::showMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    presetActions.addMenuItems (menu);
    menu.addSeparator();
    menu.addItem ("Manual", showManual);
    menu.addItem ("About", [this] { prompt.showMessage ("About", aboutText); });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuButton));
}
