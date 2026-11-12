#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <controls/controls.h>
#include "../shared/ParameterSpecs.h"
#include "RotarySlider.h"

class MasterMeter : public juce::Component
{
public:
    MasterMeter(PeakMeter& outputMeter,
                juce::AudioProcessorValueTreeState& apvts)
        : meter(outputMeter)
    {
        addAndMakeVisible(gainSlider);

        addAndMakeVisible(meter);

        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, ParameterSpecs::masterGain.id, gainSlider);
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
        auto bounds = getLocalBounds().reduced(0, 5);

        constexpr int knobSize = 44;
        constexpr int leftPadding = 6;
        constexpr int controlGap = 6;

        auto knobArea = juce::Rectangle<int>(bounds.getX() + leftPadding,
                                             bounds.getY() + (bounds.getHeight() - knobSize) / 2,
                                             knobSize,
                                             knobSize);

        auto meterArea = juce::Rectangle<int>(knobArea.getRight() + controlGap,
                                              bounds.getY(),
                                              LevelMeter::preferredWidth,
                                              bounds.getHeight());

        gainSlider.setBounds(knobArea);
        meter.setBounds(meterArea);
    }

private:
    RotarySlider gainSlider;
    LevelMeter meter;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterMeter)
};

