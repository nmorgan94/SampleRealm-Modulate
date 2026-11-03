#include "Parameters.h"

namespace
{
    std::unique_ptr<juce::RangedAudioParameter> makeParameter (const ParameterSpecs::Spec& spec)
    {
        const juce::ParameterID id { spec.id, Parameters::versionHint };

        switch (spec.kind)
        {
            case ParameterSpecs::Kind::integer:
                return std::make_unique<juce::AudioParameterInt> (
                    id, spec.name, juce::roundToInt (spec.min), juce::roundToInt (spec.max),
                    juce::roundToInt (spec.defaultValue),
                    juce::AudioParameterIntAttributes().withLabel (spec.unit));

            case ParameterSpecs::Kind::choice:
                return std::make_unique<juce::AudioParameterChoice> (
                    id, spec.name, juce::StringArray (spec.choices.data(), static_cast<int> (spec.choices.size())),
                    juce::roundToInt (spec.defaultValue));

            case ParameterSpecs::Kind::real:
                break;
        }

        return std::make_unique<juce::AudioParameterFloat> (
            id, spec.name, juce::NormalisableRange<float> (spec.min, spec.max, spec.step, spec.skew),
            spec.defaultValue, juce::AudioParameterFloatAttributes().withLabel (spec.unit));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Parameters::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto* spec : ParameterSpecs::all)
        layout.add (makeParameter (*spec));

    return layout;
}
