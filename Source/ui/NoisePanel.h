#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"

class AudioPluginAudioProcessor;

class NoisePanel : public juce::Component,
                   private juce::Button::Listener
{
public:
    explicit NoisePanel(AudioPluginAudioProcessor& proc)
        : levelSlider(proc, ParameterSpecs::noiseLevel.id)
    {
        titleLabel.setText("NOISE", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);

        enableButton.addListener(this);
        addAndMakeVisible(enableButton);

        addChoices(typeSelector, ParameterSpecs::noiseType);
        addAndMakeVisible(typeSelector);

        levelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(levelSlider);

        auto& apvts = proc.getAPVTS();
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParameterSpecs::noiseEnable.id, enableButton);
        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParameterSpecs::noiseType.id, typeSelector);
        levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::noiseLevel.id, levelSlider);

        updateEnabledAppearance();
    }

    ~NoisePanel() override
    {
        enableButton.removeListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
    }

    void resized() override
    {
        auto row = getLocalBounds().reduced(10).removeFromTop(30);

        enableButton.setBounds(row.removeFromLeft(30));
        row.removeFromLeft(3);
        titleLabel.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(5);

        const int knobSize = 30;
        typeSelector.setBounds(row.removeFromLeft(row.getWidth() - knobSize - 5));
        row.removeFromLeft(5);
        levelSlider.setBounds(row.removeFromLeft(knobSize).withHeight(knobSize));
    }

private:
    void buttonClicked(juce::Button*) override
    {
        updateEnabledAppearance();
    }

    void updateEnabledAppearance()
    {
        const bool isEnabled = enableButton.getToggleState();
        const float alpha = isEnabled ? 1.0f : 0.3f;

        titleLabel.setAlpha(alpha);
        typeSelector.setAlpha(alpha);
        typeSelector.setEnabled(isEnabled);
        levelSlider.setAlpha(alpha);
        levelSlider.setEnabled(isEnabled);

        repaint();
    }

    juce::Label titleLabel;
    juce::ToggleButton enableButton;
    juce::ComboBox typeSelector;
    ModulatableSlider levelSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoisePanel)
};
