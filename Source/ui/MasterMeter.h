#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/PeakMeter.h"
#include "../shared/ParameterSpecs.h"
#include "MeterBallistics.h"
#include "ValueLabelSlider.h"

class OutputMeter : public juce::Component,
                    private juce::Timer
{
public:
    explicit OutputMeter(PeakMeter& source)
        : meter(source)
    {
        // Drop anything played while no meter was showing
        meter.reset();

        startTimerHz(MeterBallistics::updateHz);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto meterBounds = bounds.reduced(0.0f, 0.0f);

        constexpr float gap = 2.0f;
        constexpr float channelWidth = 8.0f;
        const float totalWidth = channelWidth * 2.0f + gap;
        const float startX = meterBounds.getCentreX() - totalWidth * 0.5f;

        auto leftMeter = juce::Rectangle<float>(startX, meterBounds.getY(), channelWidth, meterBounds.getHeight());
        auto rightMeter = juce::Rectangle<float>(leftMeter.getRight() + gap, meterBounds.getY(), channelWidth, meterBounds.getHeight());

        drawChannel(g, leftMeter, bars[0]);
        drawChannel(g, rightMeter, bars[1]);
    }

private:
    void timerCallback() override
    {
        bool changed = false;

        for (int channel = 0; channel < PeakMeter::maxChannels; ++channel)
        {
            const auto peak = meter.readAndReset(channel);
            changed |= bars[static_cast<size_t>(channel)].update(peak);
        }

        if (changed)
            repaint();
    }

    static void drawChannel(juce::Graphics& g, juce::Rectangle<float> area, const MeterBallistics& bar)
    {
        const auto level = bar.fraction();

        g.setColour(juce::Colour(0xff050505));
        g.fillRect(area);
        g.setColour(juce::Colour(0xff303030));
        g.drawRect(area.toNearestInt(), 1);

        if (level > 0.0001f)
        {
            auto filled = area.reduced(1.0f);
            filled.removeFromTop(filled.getHeight() * (1.0f - level));

            juce::ColourGradient gradient(juce::Colour(0xff1ccad8), filled.getCentreX(), filled.getBottom(),
                                          juce::Colour(0xff00ff41), filled.getCentreX(), filled.getY(), false);
            g.setGradientFill(gradient);
            g.fillRect(filled);
        }

        if (bar.isClipping())
        {
            auto clipIndicator = area.withHeight(3.0f);
            g.setColour(juce::Colours::red);
            g.fillRect(clipIndicator);
        }
    }

    PeakMeter& meter;
    std::array<MeterBallistics, PeakMeter::maxChannels> bars;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputMeter)
};

class MasterMeter : public juce::Component
{
public:
    MasterMeter(PeakMeter& outputMeter,
                juce::AudioProcessorValueTreeState& apvts)
        : gainSlider(),
          meter(outputMeter)
    {
        gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
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
        constexpr int meterWidth = 26;

        auto knobArea = juce::Rectangle<int>(bounds.getX() + leftPadding,
                                             bounds.getY() + (bounds.getHeight() - knobSize) / 2,
                                             knobSize,
                                             knobSize);

        auto meterArea = juce::Rectangle<int>(knobArea.getRight() + controlGap,
                                              bounds.getY(),
                                              meterWidth,
                                              bounds.getHeight());

        gainSlider.setBounds(knobArea);
        meter.setBounds(meterArea);
    }

private:
    ValueLabelSlider gainSlider;
    OutputMeter meter;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterMeter)
};

