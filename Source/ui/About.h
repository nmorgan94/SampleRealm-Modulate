#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Mirrored by webui/src/about.ts
namespace About
{
    extern const char* const commit;

    /** "VST3 in Ableton Live", or the format alone when the host is unknown. */
    juce::String running (const juce::AudioProcessor&);

    juce::String text (const juce::AudioProcessor&);
}
