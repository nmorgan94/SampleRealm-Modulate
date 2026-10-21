#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    template <typename ParameterType>
    ParameterType* getTyped (AudioPluginAudioProcessor& processor, const char* paramID)
    {
        auto* parameter = dynamic_cast<ParameterType*> (processor.getAPVTS().getParameter (paramID));
        REQUIRE (parameter != nullptr);
        return parameter;
    }

    // Plays a chord for the same length at any block size, so tiny blocks still get past the attack
    void playChord (AudioPluginAudioProcessor& processor, int numSamples)
    {
        constexpr int totalSamples = blockSize * 4;
        int firstNonFiniteBlock = -1;

        for (int start = 0; start < totalSamples; start += numSamples)
        {
            juce::AudioBuffer<float> buffer (2, numSamples);
            juce::MidiBuffer midi;

            if (start == 0)
                for (const auto note : { 48, 55, 60, 64, 67 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, 1.0f), 0);

            processor.processBlock (buffer, midi);

            if (firstNonFiniteBlock < 0 && ! allSamplesFinite (buffer))
                firstNonFiniteBlock = start;
        }

        // One check per run, still naming the first block that went bad
        CAPTURE (firstNonFiniteBlock);
        CHECK (firstNonFiniteBlock == -1);
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Every parameter starts at its default", "[parameters]")
{
    for (auto* parameter : processor.getParameters())
    {
        CAPTURE (parameter->getName (100));
        CHECK_THAT (parameter->getValue(), WithinAbs (parameter->getDefaultValue(), 1.0e-6));
        CHECK (parameter->getDefaultValue() >= 0.0f);
        CHECK (parameter->getDefaultValue() <= 1.0f);
    }
}

TEST_CASE_METHOD (ProcessorFixture, "Parameters have the expected ranges and defaults", "[parameters]")
{
    SECTION ("Master Gain")
    {
        auto* parameter = getTyped<juce::AudioParameterFloat> (processor, "master_gain");
        CHECK (parameter->getName (100) == "Master Gain");
        CHECK_THAT (parameter->range.start, WithinAbs (-24.0, 1.0e-6));
        CHECK_THAT (parameter->range.end, WithinAbs (6.0, 1.0e-6));
        CHECK_THAT (parameter->get(), WithinAbs (0.0, 1.0e-6));
    }

    SECTION ("Filter Cutoff")
    {
        auto* parameter = getTyped<juce::AudioParameterFloat> (processor, "filter_cutoff");
        CHECK_THAT (parameter->range.start, WithinAbs (20.0, 1.0e-6));
        CHECK_THAT (parameter->range.end, WithinAbs (20000.0, 1.0e-6));
        CHECK_THAT (parameter->get(), WithinAbs (1000.0, 1.0e-3));
    }

    SECTION ("Voices and unison")
    {
        auto* voices = getTyped<juce::AudioParameterInt> (processor, "num_voices");
        CHECK (voices->getRange() == juce::Range<int> (1, Parameters::maxVoices));
        CHECK (voices->get() == Parameters::maxVoices);

        auto* unison = getTyped<juce::AudioParameterInt> (processor, "unison_voices");
        CHECK (unison->getRange() == juce::Range<int> (1, Parameters::maxUnison));
        CHECK (unison->get() == 1);
    }

    SECTION ("Switches are Off/On choices with the right defaults")
    {
        const auto [paramID, defaultOn] = GENERATE (table<const char*, bool> ({
            { "osc1_enable",   true  },
            { "osc2_enable",   false },
            { "osc3_enable",   false },
            { "noise_enable",  false },
            { "lfo1_sync",     false },
            { "filter_enable", true  },
            { "drive_hq",      false },
        }));

        CAPTURE (paramID);
        auto* parameter = getTyped<juce::AudioParameterChoice> (processor, paramID);
        CHECK (parameter->choices == juce::StringArray { "Off", "On" });
        CHECK (parameter->getIndex() == (defaultOn ? 1 : 0));
        CHECK (processor.getBoolParam (paramID) == defaultOn);
    }

    SECTION ("Filter Mode")
    {
        auto* parameter = getTyped<juce::AudioParameterChoice> (processor, "filter_mode");
        CHECK (parameter->choices == juce::StringArray { "Lowpass", "Highpass", "Bandpass", "Notch" });
        CHECK (parameter->getIndex() == 0);
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Only mono or stereo output layouts are supported", "[buses]")
{
    using Set = juce::AudioChannelSet;

    // A synth has no input bus, so each layout is { {}, { output bus } }
    CHECK (processor.isBusesLayoutSupported ({ {}, { Set::mono() } }));
    CHECK (processor.isBusesLayoutSupported ({ {}, { Set::stereo() } }));

    CHECK_FALSE (processor.isBusesLayoutSupported ({ {}, { Set::create5point1() } }));
    CHECK_FALSE (processor.isBusesLayoutSupported ({ {}, { Set::quadraphonic() } }));
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "State round trips into a new instance", "[state]")
{
    setParameter (processor, "master_gain", -12.0f);
    setParameter (processor, "filter_cutoff", 2500.0f);
    setParameter (processor, "osc2_enable", 1.0f);
    processor.getModulationManager().setAssignment ("filter_cutoff",
        { ModulationManager::ModulationSource::LFO, 2, 0.25f, 0.75f });

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    CHECK_THAT (getParameter (restored, "master_gain"), WithinAbs (-12.0, 1.0e-4));
    CHECK_THAT (getParameter (restored, "filter_cutoff"), WithinAbs (2500.0, 1.0));
    CHECK (restored.getBoolParam ("osc2_enable"));

    const auto assignment = restored.getModulationManager().getAssignment ("filter_cutoff");
    CHECK (assignment.isLFO());
    CHECK (assignment.sourceIndex == 2);
    CHECK_THAT (assignment.minValue, WithinAbs (0.25, 1.0e-6));
    CHECK_THAT (assignment.maxValue, WithinAbs (0.75, 1.0e-6));
}

TEST_CASE_METHOD (ProcessorFixture, "Invalid state is ignored", "[state]")
{
    setParameter (processor, "master_gain", -3.0f);
    processor.getModulationManager().setAssignment ("filter_cutoff",
        { ModulationManager::ModulationSource::Envelope, 1, 0.0f, 1.0f });

    SECTION ("Random bytes")
    {
        const char garbage[] = "definitely not plugin state";
        processor.setStateInformation (garbage, static_cast<int> (sizeof (garbage)));
    }

    SECTION ("Empty data")
    {
        processor.setStateInformation (nullptr, 0);
    }

    SECTION ("Valid XML with the wrong root tag")
    {
        juce::MemoryBlock state;
        juce::AudioProcessor::copyXmlToBinary (juce::XmlElement ("SomethingElse"), state);
        processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    }

    CHECK_THAT (getParameter (processor, "master_gain"), WithinAbs (-3.0, 1.0e-4));

    const auto assignment = processor.getModulationManager().getAssignment ("filter_cutoff");
    CHECK (assignment.isEnvelope());
    CHECK (assignment.sourceIndex == 1);
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Output stays finite across block sizes and settings", "[dsp]")
{
    // Includes a block larger than the size given to prepareToPlay, which some hosts send
    const auto numSamples = GENERATE (1, 64, blockSize, blockSize * 4);
    CAPTURE (numSamples);

    SECTION ("Defaults")
    {
        playChord (processor, numSamples);
    }

    SECTION ("All oscillators, noise, full unison and glide")
    {
        for (const auto* paramID : { "osc2_enable", "osc3_enable", "noise_enable" })
            setParameter (processor, paramID, 1.0f);

        setParameter (processor, "unison_voices", Parameters::maxUnison);
        setParameter (processor, "unison_detune", 100.0f);
        setParameter (processor, "glide_time", 0.5f);
        setParameter (processor, "master_gain", 6.0f);
        playChord (processor, numSamples);
    }

    SECTION ("Filter and drive at extremes")
    {
        const auto hq = GENERATE (0.0f, 1.0f);
        const auto mode = GENERATE (0.0f, 1.0f, 2.0f, 3.0f);
        CAPTURE (hq, mode);

        setParameter (processor, "drive", 1.0f);
        setParameter (processor, "drive_hq", hq);
        setParameter (processor, "filter_mode", mode);
        setParameter (processor, "filter_slope", 1.0f);
        setParameter (processor, "filter_resonance", 1.0f);
        setParameter (processor, "filter_cutoff", 20000.0f);
        playChord (processor, numSamples);
    }

    SECTION ("Filter off")
    {
        setParameter (processor, "filter_enable", 0.0f);
        playChord (processor, numSamples);
    }
}
