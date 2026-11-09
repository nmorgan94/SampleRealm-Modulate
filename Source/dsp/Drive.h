#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <memory>
#include <span>

//==============================================================================
class Drive
{
public:
    void prepare(int maximumBlockSize, int numChannels)
    {
        maxBlockSize = static_cast<size_t>(juce::jmax(0, maximumBlockSize));
        oversampler = std::make_unique<juce::dsp::Oversampling<float>>(static_cast<size_t>(numChannels), 1,
                                                                        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
        oversampler->initProcessing(maxBlockSize);
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
        juce::dsp::AudioBlock<float> block(buffer);

        const bool useHQ = hq && maxBlockSize > 0
                              && block.getNumChannels() == oversampler->numChannels;

        if (useHQ != currentHQ)
        {
            currentHQ = useHQ;
            if (useHQ)
                oversampler->reset();
        }

        if (driveGain <= kMinGain)
            return;

        if (! useHQ)
        {
            shapeBlock(block);
            return;
        }

        // The oversampler is sized for the prepared block, so a bigger host block goes in chunks
        const auto numSamples = block.getNumSamples();

        for (size_t start = 0; start < numSamples; start += maxBlockSize)
        {
            auto chunk = block.getSubBlock(start, juce::jmin(maxBlockSize, numSamples - start));
            shapeBlock(oversampler->processSamplesUp(chunk));
            oversampler->processSamplesDown(chunk);
        }
    }

private:
    void shapeBlock(const juce::dsp::AudioBlock<float>& block) const
    {
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
            for (auto& sample : std::span(block.getChannelPointer(channel), block.getNumSamples()))
                sample = shape(sample);
    }

    float shape(float x) const
    {
        return std::tanh(driveGain * x) * invTanh;
    }

    static constexpr float maxDriveGain = 20.0f;
    static constexpr float kMinGain     = 1.0e-4f;
    float driveGain = 0.0f;
    float invTanh   = 1.0f;
    bool  currentHQ = false;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    size_t maxBlockSize = 0;
};
