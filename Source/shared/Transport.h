#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/** The host's tempo and position for one block, with defaults for what it doesn't say. */
struct Transport
{
    double bpm = 174.0;
    bool playing = false;
    double ppq = 0.0; // the position in beats

    static Transport from (juce::AudioPlayHead* playHead)
    {
        Transport transport;

        if (playHead == nullptr)
            return transport;

        if (const auto position = playHead->getPosition())
        {
            transport.bpm = position->getBpm().orFallback (transport.bpm);
            // Playing with no position counts as stopped
            if (const auto ppq = position->getPpqPosition())
            {
                transport.playing = position->getIsPlaying();
                transport.ppq = *ppq;
            }
        }

        return transport;
    }

    // As a host moves its playhead, for the wasm engine
    void advance (int numSamples, double sampleRate)
    {
        if (playing)
            ppq += numSamples * bpm / (60.0 * sampleRate);
    }
};
