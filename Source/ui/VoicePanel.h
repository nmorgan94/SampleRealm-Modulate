#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "LabeledSlider.h"
#include "StepperDisplay.h"
#include "../Parameters.h"

class AudioPluginAudioProcessor;

class VoicePanel : public juce::Component
{
public:
    explicit VoicePanel(AudioPluginAudioProcessor& proc)
        : coarsePitchKnob(proc, ParameterSpecs::coarsePitch.id),
          glideKnob(proc, ParameterSpecs::glideTime.id)
    {
        titleLabel.setText("VOICE", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);

        voicesDisplay.setLabel("VOICES");
        voicesDisplay.setShowSign(false);
        voicesDisplay.attachToParameter(proc, ParameterSpecs::numVoices.id);
        addAndMakeVisible(voicesDisplay);

        unisonDisplay.setLabel("UNISON");
        unisonDisplay.setShowSign(false);
        unisonDisplay.attachToParameter(proc, ParameterSpecs::unisonVoices.id);
        addAndMakeVisible(unisonDisplay);

        detuneDisplay.setLabel("DETUNE");
        detuneDisplay.setShowSign(false);
        detuneDisplay.attachToParameter(proc, ParameterSpecs::unisonDetune.id);
        addAndMakeVisible(detuneDisplay);

        spreadDisplay.setLabel("SPREAD");
        spreadDisplay.setShowSign(false);
        spreadDisplay.attachToParameter(proc, ParameterSpecs::unisonSpread.id);
        addAndMakeVisible(spreadDisplay);

        coarsePitchKnob.setLabelText("Pitch");
        addAndMakeVisible(coarsePitchKnob);

        auto& apvts = proc.getAPVTS();
        coarsePitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::coarsePitch.id, coarsePitchKnob.getSlider());

        glideKnob.setLabelText("Glide");
        addAndMakeVisible(glideKnob);

        glideAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::glideTime.id, glideKnob.getSlider());

        rateButton.setButtonText("RATE");
        rateButton.getProperties().set("pillToggle", true);
        addAndMakeVisible(rateButton);

        rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParameterSpecs::glideMode.id, rateButton);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);

        titleLabel.setBounds(bounds.removeFromTop(30).removeFromLeft(100));

        bounds.removeFromTop(5);

        auto leftColumn = bounds.removeFromLeft(100);
        bounds.removeFromLeft(10); // gap between columns
        auto rightColumn = bounds;

        StepperDisplay* steppers[] = { &voicesDisplay, &unisonDisplay, &detuneDisplay, &spreadDisplay };

        const int stepperHeight = 20;
        const int slotHeight    = leftColumn.getHeight() / 4; // one slot per stepper

        for (auto* stepper : steppers)
        {
            auto slot = leftColumn.removeFromTop(slotHeight);
            stepper->setBounds(slot.withSizeKeepingCentre(slot.getWidth(), stepperHeight));
        }

        coarsePitchKnob.setBounds(rightColumn.removeFromTop(rightColumn.getHeight() / 2));
        glideKnob.setBounds(rightColumn);

        const auto glideBounds = glideKnob.getSlider().getBounds() + glideKnob.getPosition();
        rateButton.setBounds(glideBounds.getRight() + 5, glideBounds.getBottom() - 14, 32, 14);
    }

private:
    juce::Label titleLabel;
    StepperDisplay voicesDisplay;
    StepperDisplay unisonDisplay;
    StepperDisplay detuneDisplay;
    StepperDisplay spreadDisplay;
    LabeledSlider<ModulatableSlider> coarsePitchKnob;
    LabeledSlider<ModulatableSlider> glideKnob;
    juce::ToggleButton rateButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> coarsePitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rateAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicePanel)
};
