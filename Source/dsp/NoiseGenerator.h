#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class NoiseGenerator
{
public:
    enum class Type { White = 0, Pink };

    NoiseGenerator()
    {
        random.setSeedRandomly();
    }

    void reset()
    {
        b0 = b1 = b2 = 0.0f;
    }

    float getNextSample(Type type)
    {
        const float white = random.nextFloat() * 2.0f - 1.0f;

        if (type == Type::White)
            return white;

        b0 = 0.99765f * b0 + white * 0.0990460f;
        b1 = 0.96300f * b1 + white * 0.2965164f;
        b2 = 0.57000f * b2 + white * 1.0526913f;

        return (b0 + b1 + b2 + white * 0.1848f) * 0.11f;
    }

private:
    juce::Random random;
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGenerator)
};
