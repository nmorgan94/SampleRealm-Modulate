#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "StepperDisplay.h"
#include "../Parameters.h"

class AudioPluginAudioProcessor;

class VoicePanel : public juce::Component
{
public:
    VoicePanel(AudioPluginAudioProcessor& proc,
               juce::AudioProcessorValueTreeState& apvts)
        : coarsePitchSlider(proc, ParameterSpecs::coarsePitch.id),
          glideSlider(proc, ParameterSpecs::glideTime.id)
    {
        titleLabel.setText("VOICE", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);

        voicesDisplay.setLabel("VOICES");
        voicesDisplay.setShowSign(false);
        voicesDisplay.attachToParameter(proc, apvts, ParameterSpecs::numVoices.id);
        addAndMakeVisible(voicesDisplay);

        unisonDisplay.setLabel("UNISON");
        unisonDisplay.setShowSign(false);
        unisonDisplay.attachToParameter(proc, apvts, ParameterSpecs::unisonVoices.id);
        addAndMakeVisible(unisonDisplay);

        detuneDisplay.setLabel("DETUNE");
        detuneDisplay.setShowSign(false);
        detuneDisplay.attachToParameter(proc, apvts, ParameterSpecs::unisonDetune.id);
        addAndMakeVisible(detuneDisplay);

        spreadDisplay.setLabel("SPREAD");
        spreadDisplay.setShowSign(false);
        spreadDisplay.attachToParameter(proc, apvts, ParameterSpecs::unisonSpread.id);
        addAndMakeVisible(spreadDisplay);

        coarsePitchLabel.setText("Pitch", juce::dontSendNotification);
        coarsePitchLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(coarsePitchLabel);

        coarsePitchSlider.attachLabel(coarsePitchLabel);
        addAndMakeVisible(coarsePitchSlider);

        coarsePitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::coarsePitch.id, coarsePitchSlider);

        glideLabel.setText("Glide", juce::dontSendNotification);
        glideLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(glideLabel);

        glideSlider.attachLabel(glideLabel);
        addAndMakeVisible(glideSlider);

        glideAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::glideTime.id, glideSlider);
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

        auto topHalf = rightColumn.removeFromTop(rightColumn.getHeight() / 2);
        coarsePitchLabel.setBounds(topHalf.removeFromTop(15));
        auto pitchSize = juce::jmin(topHalf.getWidth(), topHalf.getHeight());
        coarsePitchSlider.setBounds(topHalf.withSizeKeepingCentre(pitchSize, pitchSize));

        glideLabel.setBounds(rightColumn.removeFromTop(15));
        auto glideSize = juce::jmin(rightColumn.getWidth(), rightColumn.getHeight());
        glideSlider.setBounds(rightColumn.withSizeKeepingCentre(glideSize, glideSize));
    }

private:
    juce::Label titleLabel;
    juce::Label coarsePitchLabel;
    juce::Label glideLabel;
    StepperDisplay voicesDisplay;
    StepperDisplay unisonDisplay;
    StepperDisplay detuneDisplay;
    StepperDisplay spreadDisplay;
    ModulatableSlider coarsePitchSlider;
    ModulatableSlider glideSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> coarsePitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicePanel)
};
