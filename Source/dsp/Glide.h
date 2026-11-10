#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <cmath>

//==============================================================================
inline constexpr double semitonesPerOctave = 12.0;

inline double noteNumberToFrequencyHz(double midiNoteNumber)
{
    return 440.0 * std::exp2((midiNoteNumber - 69.0) / semitonesPerOctave);
}

//==============================================================================
// In the order of ParameterSpecs::glideModes
enum class GlideMode
{
    fixedTime,  // same time for any interval
    fixedRate   // glide time per octave
};

//==============================================================================
// A voice's pitch as a ratio to its note's
class GlideProcessor
{
public:
    void start(int midiNoteNumber, double fromNoteNumber, float glideSeconds, GlideMode mode, double sampleRate)
    {
        const double interval = fromNoteNumber - midiNoteNumber;
        const double rampSeconds = mode == GlideMode::fixedRate
                                 ? glideSeconds * std::abs(interval) / semitonesPerOctave
                                 : glideSeconds;

        note = midiNoteNumber;
        ratio.reset(sampleRate, rampSeconds);
        ratio.setCurrentAndTargetValue(std::exp2(interval / semitonesPerOctave));
        ratio.setTargetValue(1.0);
    }

    int getNoteNumber() const noexcept { return note; }
    double getNextRatio() noexcept { return ratio.getNextValue(); }

    // Highest ratio still to come
    double peakRatio() const noexcept { return std::max(ratio.getCurrentValue(), 1.0); }

    double currentNoteNumber() const { return note + semitonesPerOctave * std::log2(ratio.getCurrentValue()); }

private:
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative> ratio { 1.0 };
    int note = 0;
};

//==============================================================================
// Held keys, newest last, and the latest glide
class Legato
{
public:
    void press(int midiNoteNumber)
    {
        release(midiNoteNumber);

        if (count < keys.size())
            keys[count++] = midiNoteNumber;
    }

    void release(int midiNoteNumber)
    {
        auto* const held = keys.data() + count;
        auto* const found = std::find(keys.data(), held, midiNoteNumber);

        if (found == held)
            return;

        std::copy(found + 1, held, found);
        --count;
    }

    void clear()
    {
        count = 0;
        latest = nullptr;
    }

    bool isHeld() const noexcept { return count > 0; }
    int newest() const noexcept { return keys[count - 1]; }

    // Slides only while a key is held
    void start(GlideProcessor& glide, int midiNoteNumber, float glideSeconds, GlideMode mode, double sampleRate)
    {
        const double from = isHeld() && latest != nullptr ? latest->currentNoteNumber() : midiNoteNumber;

        glide.start(midiNoteNumber, from, glideSeconds, mode, sampleRate);
        latest = &glide;
    }

private:
    std::array<int, 128> keys {};
    size_t count = 0;
    const GlideProcessor* latest = nullptr;
};
