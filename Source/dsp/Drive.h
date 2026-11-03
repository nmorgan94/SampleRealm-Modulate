#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <span>

//==============================================================================
// juce::dsp::Oversampling's 2x polyphase IIR stage at maximum quality, with the
// coefficients its filter design gives for that stage
class HalfBandOversampler
{
public:
    static constexpr int maxChannels = 2;

    static constexpr std::array upCoefficients { 0.045728147f, 0.332501113f, 0.663202047f, 0.933855832f,
                                                 0.168087542f, 0.504485726f, 0.803780854f };
    static constexpr std::array downCoefficients { 0.0542175248f, 0.383087337f, 0.748720944f,
                                                   0.196797967f, 0.573136389f, 0.914293706f };

    void reset()
    {
        channels = {};
    }

    // Runs one sample up to 2x, through shape, and back down
    template <typename Shape>
    float process(int channel, float sample, Shape&& shape)
    {
        auto& state = channels[static_cast<size_t>(channel)];

        const auto even = shape(allpasses(upCoefficients, state.up, 0, upDirectStages, sample));
        const auto odd  = shape(allpasses(upCoefficients, state.up, upDirectStages, upCoefficients.size(), sample));

        const auto direct  = allpasses(downCoefficients, state.down, 0, downDirectStages, even);
        const auto delayed = allpasses(downCoefficients, state.down, downDirectStages, downCoefficients.size(), odd);

        const auto output = (state.delay + direct) * 0.5f;
        state.delay = delayed;
        return output;
    }

    // As JUCE does: only on Intel, where the macro isn't empty
    void snapToZero()
    {
        using juce::ignoreUnused;

        for (auto& state : channels)
        {
            for (auto& value : state.up)
                JUCE_SNAP_TO_ZERO (value);

            for (auto& value : state.down)
                JUCE_SNAP_TO_ZERO (value);
        }
    }

private:
    static constexpr size_t upDirectStages = upCoefficients.size() - upCoefficients.size() / 2;
    static constexpr size_t downDirectStages = downCoefficients.size() - downCoefficients.size() / 2;

    template <size_t numStages>
    static float allpasses(const std::array<float, numStages>& coefficients, std::array<float, numStages>& state,
                           size_t first, size_t end, float input)
    {
        for (auto n = first; n < end; ++n)
        {
            const auto alpha = coefficients[n];
            const auto output = alpha * input + state[n];
            state[n] = input - alpha * output;
            input = output;
        }

        return input;
    }

    struct Channel
    {
        std::array<float, upCoefficients.size()> up {};
        std::array<float, downCoefficients.size()> down {};
        float delay = 0.0f;
    };

    std::array<Channel, maxChannels> channels {};
};

//==============================================================================
class Drive
{
public:
    void prepare(int maximumBlockSize, int numChannels)
    {
        oversamplerChannels = juce::jmin(numChannels, HalfBandOversampler::maxChannels);
        maxBlockSize = maximumBlockSize;
        oversampler.reset();
        currentHQ = false;
    }

    void setDrive(float amount01)                  // 0..1 from the (modulated) param
    {
        const float a = juce::jlimit(0.0f, 1.0f, amount01);
        driveGain = a * maxDriveGain;              // pre-gain into the shaper
        invTanh   = driveGain > kMinGain ? 1.0f / std::tanh(driveGain) : 1.0f;
    }

    void process(juce::AudioBuffer<float>& buffer, bool hq)
    {
        const bool useHQ = hq && maxBlockSize > 0
                              && buffer.getNumChannels() == oversamplerChannels;

        if (useHQ != currentHQ)
        {
            currentHQ = useHQ;
            if (useHQ)
                oversampler.reset();
        }

        if (driveGain <= kMinGain)
            return;

        const auto numSamples = buffer.getNumSamples();
        const auto shaper = [this](float x) { return shape(x); };

        if (! useHQ)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (auto& sample : std::span(buffer.getWritePointer(channel), static_cast<size_t>(numSamples)))
                    sample = shape(sample);

            return;
        }

        // In chunks of the prepared block size, as juce::dsp::Oversampling has to
        for (int start = 0; start < numSamples; start += maxBlockSize)
        {
            const auto chunk = static_cast<size_t>(juce::jmin(maxBlockSize, numSamples - start));

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (auto& sample : std::span(buffer.getWritePointer(channel, start), chunk))
                    sample = oversampler.process(channel, sample, shaper);

            oversampler.snapToZero();
        }
    }

private:
    float shape(float x) const
    {
        return std::tanh(driveGain * x) * invTanh;
    }

    static constexpr float maxDriveGain = 20.0f;
    static constexpr float kMinGain     = 1.0e-4f;
    float driveGain = 0.0f;
    float invTanh   = 1.0f;
    bool  currentHQ = false;

    HalfBandOversampler oversampler;
    int oversamplerChannels = 0;
    int maxBlockSize = 0;
};
