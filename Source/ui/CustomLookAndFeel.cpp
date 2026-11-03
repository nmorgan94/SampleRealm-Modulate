#include "CustomLookAndFeel.h"
#include <BinaryData.h>
#include <map>

namespace
{
    // FontOptions can't take both a typeface and variable settings, so each weight is its own clone
    juce::Typeface::Ptr orbitronTypeface (float weight)
    {
        static const auto variable = juce::Typeface::createSystemTypefaceFor (
            BinaryData::OrbitronVariable_ttf, BinaryData::OrbitronVariable_ttfSize);

        static juce::CriticalSection lock;
        static std::map<float, juce::Typeface::Ptr> byWeight;

        const juce::ScopedLock scope (lock);
        auto& typeface = byWeight[weight];

        if (typeface == nullptr)
        {
            const juce::FontVariableSetting setting { "wght", weight };
            typeface = variable->cloneWithVariableSettings ({ &setting, 1 });
        }

        return typeface;
    }
}

//==============================================================================
juce::FontOptions CustomLookAndFeel::orbitron (float weight)
{
    return juce::FontOptions (orbitronTypeface (weight));
}
