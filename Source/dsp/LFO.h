#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "LfoCurve.h"

class LFO
{
public:
    LFO() = default;
    
    void prepare(double sr)
    {
        sampleRate = sr;
        phase = 0.0f;
    }
    
    void setRate(float rateHz)
    {
        rate = juce::jlimit(0.01f, 100.0f, rateHz);
    }
    
    void setTriggerMode(bool shouldTrigger)
    {
        triggerMode = shouldTrigger;
    }
    
    bool isTriggerMode() const
    {
        return triggerMode;
    }
    
    void setTempoSync(bool shouldSync)
    {
        tempoSyncEnabled = shouldSync;
    }
    
    void setTempoDivision(int divisionIndex)
    {
        tempoDivision = juce::jlimit(0, 14, divisionIndex);
    }
    
    bool isTempoSynced() const
    {
        return tempoSyncEnabled;
    }

    // The phase at the host's position
    void syncTo(double ppq)
    {
        const double cycles = ppq / beatsPerCycle[tempoDivision];
        phase = static_cast<float>(cycles - std::floor(cycles));

        // Just below a whole cycle rounds up to 1 as a float
        if (phase >= 1.0f)
            phase = 0.0f;
    }

    void trigger()
    {
        if (triggerMode)
        {
            phase = 0.0f;
        }
    }
    
    void setShape(std::span<const LfoCurve::Point> points, float tension)
    {
        for (size_t i = 0; i < lookupTableSize; ++i)
        {
            float x = i / static_cast<float>(lookupTableSize - 1);
            lookupTable[i] = LfoCurve::evaluate(points, tension, x);
        }
    }
    
    // Get current LFO value (0.0 to 1.0) - audio thread safe
    float getCurrentValue() const
    {
        // Linear interpolation in lookup table
        float index = phase * (lookupTableSize - 1);
        size_t i0 = static_cast<size_t>(index);
        size_t i1 = juce::jmin(i0 + 1, static_cast<size_t>(lookupTableSize - 1));
        float frac = index - i0;
        
        return lookupTable[i0] + frac * (lookupTable[i1] - lookupTable[i0]);
    }
    
    float getCurrentPhase() const
    {
        return phase;
    }
    
    // Advance the LFO by the specified number of samples
    void advance(int numSamples, double bpm)
    {
        if (sampleRate <= 0.0 || numSamples <= 0)
            return;

        float effectiveRate = rate;

        if (tempoSyncEnabled && bpm > 0.0)
            effectiveRate = static_cast<float>(bpm / (60.0 * beatsPerCycle[tempoDivision]));

        float phaseIncrement = effectiveRate / static_cast<float>(sampleRate);
        phase += phaseIncrement * numSamples;
        
        // Wrap phase to 0.0-1.0 range using fmod
        phase = std::fmod(phase, 1.0f);
    }

private:
    static constexpr size_t lookupTableSize = 512;
    std::array<float, lookupTableSize> lookupTable{};  // Zero-initialized
    
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float rate = 1.0f;  // Hz
    bool triggerMode = true;
    bool tempoSyncEnabled = false;
    int tempoDivision = 6;

    static constexpr double beatsPerCycle[] = {
        0.25,                   // 1/16
        0.25 * 2.0 / 3.0,       // 1/16T
        0.25 * 1.5,             // 1/16D
        0.5,                    // 1/8
        0.5 * 2.0 / 3.0,        // 1/8T
        0.5 * 1.5,              // 1/8D
        1.0,                    // 1/4
        1.0 * 2.0 / 3.0,        // 1/4T
        1.0 * 1.5,              // 1/4D
        2.0,                    // 1/2
        2.0 * 2.0 / 3.0,        // 1/2T
        2.0 * 1.5,              // 1/2D
        4.0,                    // 1 Bar
        8.0,                    // 2 Bars
        16.0                    // 4 Bars
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO)
};
