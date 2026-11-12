#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>
#include "shared/ParameterSpecs.h"

namespace Parameters
{
    inline constexpr int versionHint = 1;

    // Where the parameter is in ParameterSpecs::all
    inline std::optional<size_t> indexOf (const juce::String& id)
    {
        const auto found = std::ranges::find_if (ParameterSpecs::all, [&id] (const auto* spec) { return id == spec->id; });

        if (found == ParameterSpecs::all.end())
            return {};

        return static_cast<size_t> (found - ParameterSpecs::all.begin());
    }
}
