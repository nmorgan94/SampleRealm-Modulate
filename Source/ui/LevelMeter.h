#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/PeakMeter.h"

//==============================================================================
/** A narrow peak meter with one bar per channel, each with a clip light on top. Bars rise at
    once and fall smoothly, and clip lights hold for a moment. */
class LevelMeter : public juce::Component,
                   private juce::Timer
{
public:
    enum ColourIds
    {
        backgroundColourId = 0x5d10300,
        outlineColourId,
        barColourId,
        clipColourId
    };

    static constexpr int barWidth = 12;
    static constexpr int gap = 3;
    static constexpr int preferredWidth = barWidth * PeakMeter::maxChannels + gap * (PeakMeter::maxChannels - 1);

    //==============================================================================
    /** What one bar shows. update() is called at updateHz with the peak since the last call. */
    struct Ballistics
    {
        static constexpr int updateHz = 30;
        static constexpr int clipHoldTicks = 2 * updateHz;
        static constexpr float floorDb = -60.0f;
        static constexpr float fallDbPerTick = 20.0f / updateHz;

        float levelDb = floorDb;
        int clipTicks = 0;

        bool isClipping() const noexcept   { return clipTicks > 0; }

        /** Takes a peak as linear gain. Returns true if what the bar shows has changed. */
        bool update (float peak) noexcept
        {
            const auto wasClipping = isClipping();

            if (peak > 1.0f)
                clipTicks = clipHoldTicks;
            else if (clipTicks > 0)
                --clipTicks;

            // Capped at the top of the scale, so even an infinite peak can still fall away
            const auto newLevel = juce::jmin (0.0f, juce::jmax (juce::Decibels::gainToDecibels (peak, floorDb),
                                                               levelDb - fallDbPerTick));

            const auto changed = ! juce::approximatelyEqual (newLevel, levelDb) || wasClipping != isClipping();
            levelDb = newLevel;
            return changed;
        }
    };

    //==============================================================================
    explicit LevelMeter (PeakMeter& source) : meter (source)
    {
        // Drop anything played while no meter was showing
        meter.reset();

        startTimerHz (Ballistics::updateHz);
    }

    void paint (juce::Graphics& g) override
    {
        const auto numChannels = meter.getNumChannels();
        auto area = getLocalBounds().withSizeKeepingCentre (widthFor (numChannels), getHeight());

        for (int channel = 0; channel < numChannels; ++channel)
        {
            paintBar (g, area.removeFromLeft (barWidth).toFloat(), bar (channel));
            area.removeFromLeft (gap);
        }
    }

private:
    PeakMeter& meter;
    std::array<Ballistics, PeakMeter::maxChannels> bars;
    int barsShown = 0;

    static constexpr int widthFor (int numChannels)   { return barWidth * numChannels + gap * (numChannels - 1); }

    Ballistics& bar (int channel)   { return bars[static_cast<size_t> (channel)]; }

    void paintBar (juce::Graphics& g, juce::Rectangle<float> bounds, const Ballistics& state) const
    {
        bounds = bounds.reduced (0.5f);
        const auto light = bounds.removeFromTop (6.0f);
        bounds.removeFromTop (3.0f);

        g.setColour (colour (state.isClipping() ? clipColourId : outlineColourId));
        g.fillRoundedRectangle (light, 2.0f);

        g.setColour (colour (backgroundColourId));
        g.fillRoundedRectangle (bounds, 2.0f);

        const auto fraction = juce::jmap (state.levelDb, Ballistics::floorDb, 0.0f, 0.0f, 1.0f);
        auto fill = bounds.reduced (1.0f);
        g.setColour (colour (barColourId));
        g.fillRoundedRectangle (fill.removeFromBottom (fill.getHeight() * fraction), 1.5f);

        g.setColour (colour (outlineColourId));
        g.drawRoundedRectangle (bounds, 2.0f, 1.0f);
    }

    // Falls back to standard LookAndFeel_V4 colours, so the meter follows any colour scheme
    juce::Colour colour (int id) const
    {
        if (isColourSpecified (id) || getLookAndFeel().isColourSpecified (id))
            return findColour (id);

        switch (id)
        {
            case backgroundColourId: return findColour (juce::Slider::backgroundColourId);
            case outlineColourId:    return findColour (juce::ComboBox::outlineColourId);
            case barColourId:        return findColour (juce::Slider::thumbColourId);
            default:                 return juce::Colour (0xffff3d4a);
        }
    }

    void timerCallback() override
    {
        const auto numChannels = meter.getNumChannels();
        auto changed = std::exchange (barsShown, numChannels) != numChannels;

        for (int channel = 0; channel < numChannels; ++channel)
            changed |= bar (channel).update (meter.readAndReset (channel));

        if (changed)
            repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
