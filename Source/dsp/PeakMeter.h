#pragma once

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
 * Collects the loudest sample of each channel on the audio thread for a meter to read on the
 * UI thread. It keeps the maximum since the last read, so short transients are never missed.
 */
class PeakMeter
{
public:
    static constexpr int maxChannels = 2;

    explicit PeakMeter (int numChannelsToMeter)   { setNumChannels (numChannelsToMeter); }

    void setNumChannels (int numChannelsToMeter) noexcept
    {
        numChannels = juce::jlimit (1, maxChannels, numChannelsToMeter);
    }

    /** Channels beyond getNumChannels() are ignored. */
    void process (const juce::AudioBuffer<float>& buffer) noexcept
    {
        const auto channelsToRead = juce::jmin (getNumChannels(), buffer.getNumChannels());

        for (int channel = 0; channel < channelsToRead; ++channel)
        {
            const auto blockPeak = buffer.getMagnitude (channel, 0, buffer.getNumSamples());

            auto& channelPeak = peak (channel);
            auto current = channelPeak.load();
            while (blockPeak > current && ! channelPeak.compare_exchange_weak (current, blockPeak)) {}
        }
    }

    /** The loudest sample on a channel since the last call, as linear gain. */
    float readAndReset (int channel) noexcept
    {
        jassert (juce::isPositiveAndBelow (channel, maxChannels));
        return peak (channel).exchange (0.0f);
    }

    void reset() noexcept
    {
        for (auto& channelPeak : peaks)
            channelPeak.store (0.0f);
    }

    int getNumChannels() const noexcept         { return numChannels.load(); }

private:
    static_assert (std::atomic<float>::is_always_lock_free && std::atomic<int>::is_always_lock_free,
                   "PeakMeter is used on the audio thread");

    std::atomic<int> numChannels;
    std::array<std::atomic<float>, maxChannels> peaks {};

    std::atomic<float>& peak (int channel) noexcept   { return peaks[static_cast<size_t> (channel)]; }

    JUCE_DECLARE_NON_COPYABLE (PeakMeter)
};
