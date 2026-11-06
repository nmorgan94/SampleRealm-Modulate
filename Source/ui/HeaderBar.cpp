#include "HeaderBar.h"
#include "CustomLookAndFeel.h"

namespace
{
    constexpr int edge = 10;
    constexpr int gap = 12;
    constexpr int titleWidth = 400;
    constexpr int versionTop = 60, versionHeight = 12;
    constexpr int meterWidth = 90;
    constexpr int controlsHeight = 30;
}

HeaderBar::HeaderBar (const juce::String& titleText, PresetManager& presets, PromptOverlay& o, std::function<void()> m,
                      const juce::String& about, PeakMeter& source, juce::AudioProcessorValueTreeState& apvts)
    : prompt (o), showManual (std::move (m)), aboutText (about),
      presetActions (presets, prompt), presetBar (presets, presetActions), meter (source, apvts)
{
    title.setText (titleText, juce::dontSendNotification);
    title.setFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::boldWeight).withHeight (36.0f));
    title.setColour (juce::Label::textColourId, juce::Colour (0xff00ff00));
    title.setJustificationType (juce::Justification::centredLeft);
    title.setMinimumHorizontalScale (0.7f);
    title.setBorderSize ({});

    version.setText ("v" JucePlugin_VersionString, juce::dontSendNotification);
    version.setFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::regularWeight).withPointHeight (9.0f));
    version.setColour (juce::Label::textColourId, juce::Colour (0xff00ff41).withAlpha (0.4f));
    version.setJustificationType (juce::Justification::centredLeft);
    version.setBorderSize ({});

    menuButton.onClick = [this] { showMenu(); };

    addAndMakeVisible (title);
    addAndMakeVisible (version);
    addAndMakeVisible (presetBar);
    addAndMakeVisible (menuButton);
    addAndMakeVisible (meter);
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced (edge, 0);

    title.setBounds (bounds.withWidth (titleWidth));
    version.setBounds (bounds.getX(), versionTop, titleWidth, versionHeight);
    meter.setBounds (bounds.removeFromRight (meterWidth).reduced (0, 6));

    auto controls = bounds.withTrimmedLeft (titleWidth + gap).withTrimmedRight (gap);
    controls = controls.withSizeKeepingCentre (controls.getWidth(), controlsHeight);
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
