#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/** What one meter bar shows. update() is called at updateHz with the peak since the last call. Bars rise at once and fall smoothly, and the clip light
    holds for a moment. */
struct MeterBallistics
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

    /** How full the bar is, 0 to 1. */
    float fraction() const noexcept   { return juce::jmap (levelDb, floorDb, 0.0f, 0.0f, 1.0f); }
};
