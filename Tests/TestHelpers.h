#pragma once

#include <catch2/catch_test_macros.hpp>

#include "PluginProcessor.h"

//==============================================================================
namespace TestHelpers
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    // Sets a parameter in its real units (e.g. dB) the way a host would
    inline void setParameter (AudioPluginAudioProcessor& processor,
                              const juce::ParameterID& id,
                              float value)
    {
        auto* parameter = processor.getAPVTS().getParameter (id.getParamID());
        REQUIRE (parameter != nullptr);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    inline float getParameter (AudioPluginAudioProcessor& processor, const juce::ParameterID& id)
    {
        auto* value = processor.getAPVTS().getRawParameterValue (id.getParamID());
        REQUIRE (value != nullptr);
        return value->load();
    }

    // Fills every channel with the same constant value, which makes gain easy to check
    inline juce::AudioBuffer<float> makeConstantBuffer (int numChannels, int numSamples, float value)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);

        for (int channel = 0; channel < numChannels; ++channel)
            juce::FloatVectorOperations::fill (buffer.getWritePointer (channel), value, numSamples);

        return buffer;
    }

    inline juce::AudioBuffer<float> makeSineBuffer (int numChannels, int numSamples,
                                                    float frequency, float amplitude)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        const auto increment = juce::MathConstants<double>::twoPi * frequency / sampleRate;

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (channel, i, amplitude * static_cast<float> (std::sin (increment * i)));

        return buffer;
    }

    inline void process (AudioPluginAudioProcessor& processor, juce::AudioBuffer<float>& buffer)
    {
        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);
    }

    inline bool allSamplesFinite (const juce::AudioBuffer<float>& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                if (! std::isfinite (buffer.getSample (channel, i)))
                    return false;

        return true;
    }

    //==============================================================================
    // A prepared processor, as a host would have it just before playback starts
    struct ProcessorFixture
    {
        ProcessorFixture()
        {
            // A host sets these before preparing, and a processor may read them back through
            // getSampleRate() while preparing. Without this they stay 0
            processor.setRateAndBufferSizeDetails (sampleRate, blockSize);
            processor.prepareToPlay (sampleRate, blockSize);
        }

        ~ProcessorFixture()
        {
            processor.releaseResources();
        }

        // Parameters notify listeners on the message thread, so JUCE must be initialised
        juce::ScopedJuceInitialiser_GUI juceInitialiser;
        AudioPluginAudioProcessor processor;
    };
}
