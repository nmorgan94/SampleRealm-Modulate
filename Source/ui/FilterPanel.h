#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "LabeledSlider.h"
#include "FilterResponseDisplay.h"

class AudioPluginAudioProcessor;

class FilterPanel : public juce::Component,
                    private juce::Button::Listener,
                    private juce::ComboBox::Listener,
                    private juce::Timer
{
public:
    explicit FilterPanel(AudioPluginAudioProcessor& proc)
        : processor(proc),
          driveKnob(proc, ParameterSpecs::drive.id),
          cutoffKnob(proc, ParameterSpecs::filterCutoff.id),
          resonanceKnob(proc, ParameterSpecs::filterResonance.id)
    {
        titleLabel.setText("FILTER", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);
        
        enableButton.addListener(this);
        addAndMakeVisible(enableButton);
        
        modeLabel.setText("Mode", juce::dontSendNotification);
        modeLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(modeLabel);
        
        addChoices(modeSelector, ParameterSpecs::filterMode);
        modeSelector.addListener(this);
        addAndMakeVisible(modeSelector);

        addChoices(slopeSelector, ParameterSpecs::filterSlope);
        addAndMakeVisible(slopeSelector);

        cutoffKnob.setLabelText("Cutoff");
        resonanceKnob.setLabelText("Resonance");
        driveKnob.setLabelText("Drive");
        addAndMakeVisible(cutoffKnob);
        addAndMakeVisible(resonanceKnob);
        addAndMakeVisible(driveKnob);

        hqButton.setButtonText("HQ");
        hqButton.getProperties().set("pillToggle", true);
        addAndMakeVisible(hqButton);

        auto& apvts = proc.getAPVTS();
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParameterSpecs::filterEnable.id, enableButton);
        modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParameterSpecs::filterMode.id, modeSelector);
        slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParameterSpecs::filterSlope.id, slopeSelector);
        cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::filterCutoff.id, cutoffKnob.getSlider());
        resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::filterResonance.id, resonanceKnob.getSlider());
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::drive.id, driveKnob.getSlider());
        hqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParameterSpecs::driveHq.id, hqButton);

        addAndMakeVisible(responseDisplay);

        updateEnabledAppearance();
    }
    
    ~FilterPanel() override
    {
        stopTimer();
        enableButton.removeListener(this);
        modeSelector.removeListener(this);
    }
    
    void visibilityChanged() override
    {
        if (isVisible() && enableButton.getToggleState())
            startTimerHz(30);
        else
            stopTimer();
    }
    
    void timerCallback() override
    {
        updateResponseDisplay();
    }

    void updateResponseDisplay()
    {
        bool isEnabled = enableButton.getToggleState();
        auto mode = static_cast<FilterMode>(modeSelector.getSelectedItemIndex());
        auto slope = static_cast<FilterSlope>(slopeSelector.getSelectedItemIndex());
        float cutoff = processor.getModulatedParam(ParameterSpecs::filterCutoff.id);
        float resonance = processor.getModulatedParam(ParameterSpecs::filterResonance.id);

        responseDisplay.setEnabled(isEnabled);
        responseDisplay.setFilterParameters(mode, slope, cutoff, resonance, processor.getSampleRate());
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Draw background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        
        // Draw border
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        auto topRow = bounds.removeFromTop(30);
        enableButton.setBounds(topRow.removeFromLeft(30));
        topRow.removeFromLeft(10);
        titleLabel.setBounds(topRow.removeFromLeft(80));

        bounds.removeFromTop(5);
        
        auto modeRow = bounds.removeFromTop(25);
        modeLabel.setBounds(modeRow.removeFromLeft(50));
        modeSelector.setBounds(modeRow.removeFromLeft(110));
        modeRow.removeFromLeft(5);
        slopeSelector.setBounds(modeRow.removeFromLeft(75));
        
        bounds.removeFromTop(10);
        
        auto displayArea = bounds.removeFromTop(100);
        responseDisplay.setBounds(displayArea);
        
        bounds.removeFromTop(5);
        
        auto knobArea = bounds;
        int knobWidth = knobArea.getWidth() / 3;

        driveKnob.setBounds(knobArea.removeFromLeft(knobWidth).reduced(5));
        cutoffKnob.setBounds(knobArea.removeFromLeft(knobWidth).reduced(5));
        resonanceKnob.setBounds(knobArea.reduced(5));

        auto knobBounds = driveKnob.getSlider().getBounds() + driveKnob.getPosition();
        hqButton.setBounds(knobBounds.removeFromTop(14).removeFromRight(20).translated(15, 0));
    }
    
private:
    void buttonClicked(juce::Button*) override
    {
        updateEnabledAppearance();
    }

    void comboBoxChanged(juce::ComboBox*) override
    {
        updateSlopeAvailability();
    }

    // A notch has no skirt to steepen, so the slope control goes dead in that mode.
    void updateSlopeAvailability()
    {
        const auto mode = static_cast<FilterMode>(modeSelector.getSelectedItemIndex());
        const bool usable = enableButton.getToggleState() && FilterStages::hasSlope(mode);

        slopeSelector.setAlpha(usable ? 1.0f : 0.3f);
        slopeSelector.setEnabled(usable);
    }

    void updateEnabledAppearance()
    {
        bool isEnabled = enableButton.getToggleState();
        float alpha = isEnabled ? 1.0f : 0.3f;
        
        titleLabel.setAlpha(alpha);
        modeLabel.setAlpha(alpha);
        modeSelector.setAlpha(alpha);
        modeSelector.setEnabled(isEnabled);

        updateSlopeAvailability();

        for (auto* knob : { &cutoffKnob, &resonanceKnob, &driveKnob })
        {
            knob->setAlpha(alpha);
            knob->setEnabled(isEnabled);
        }

        hqButton.setAlpha(alpha);
        hqButton.setEnabled(isEnabled);

        updateResponseDisplay();

        if (isEnabled && isVisible())
            startTimerHz(30);
        else
            stopTimer();
        
        repaint();
    }
    
    AudioPluginAudioProcessor& processor;
    
    juce::Label titleLabel;
    juce::ToggleButton enableButton;
    
    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    juce::ComboBox slopeSelector;

    LabeledSlider<ModulatableSlider> driveKnob;
    juce::ToggleButton hqButton;

    LabeledSlider<ModulatableSlider> cutoffKnob;
    LabeledSlider<ModulatableSlider> resonanceKnob;

    FilterResponseDisplay responseDisplay;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hqAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPanel)
};
