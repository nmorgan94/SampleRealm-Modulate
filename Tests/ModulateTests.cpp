#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
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
    CHECK_THAT (getParameter (restored, "osc2_enable"), WithinAbs (1.0, 1.0e-6));

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

        setParameter (processor, "unison_voices", ParameterSpecs::maxUnison);
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

TEST_CASE_METHOD (ProcessorFixture, "A tension change reaches the LFO with no editor open", "[dsp][lfo]")
{
    // LFO 1 runs free while assigned, so it moves without notes
    setParameter (processor, ParameterSpecs::lfo[0].mode.id, 1.0f);
    processor.getModulationManager().setAssignment (ParameterSpecs::filterCutoff.id,
                                                    { Modulation::Source::LFO, 0, 0.0f, 1.0f });

    setParameter (processor, ParameterSpecs::lfo[0].tension.id, 1.0f);

    for (int block = 0; block < 10; ++block)
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        process (processor, buffer);
    }

    const auto& live = processor.getLive();
    const auto phase = live.lfoPhase[0].load();
    REQUIRE (phase > 0.05f);

    // The LFO reads a 512-point table, so allow for its interpolation
    CHECK_THAT (live.lfoValue[0].load(), WithinAbs (LfoCurve::evaluate (LfoCurve::defaultPoints, 1.0f, phase), 1.0e-3));
    CHECK (std::abs (live.lfoValue[0].load() - LfoCurve::evaluate (LfoCurve::defaultPoints, 0.5f, phase)) > 1.0e-2f);
}

TEST_CASE_METHOD (ProcessorFixture, "LFO points that share an x keep the output finite", "[dsp][lfo]")
{
    processor.setLFOCurvePoints (0, { { 0.0f, 0.2f }, { 0.0f, 0.9f }, { 1.0f, 0.2f } });
    processor.getModulationManager().setAssignment (ParameterSpecs::filterCutoff.id,
                                                    { Modulation::Source::LFO, 0, 0.0f, 1.0f });

    playChord (processor, blockSize);
}

TEST_CASE_METHOD (ProcessorFixture, "Editors see a curve's points only up to what the DSP keeps", "[lfo][state]")
{
    std::vector<LfoCurve::Point> points;

    for (auto x = 0.0f; points.size() <= LfoCurve::maxPoints; x += 0.01f)
        points.emplace_back (x, 0.5f);

    processor.setLFOCurvePoints (0, points);

    CHECK (processor.getLFOCurvePoints (0).size() == LfoCurve::maxPoints);
    CHECK (processor.getLFOCurves()[0].count == LfoCurve::maxPoints);
}

TEST_CASE ("Modulated values stay inside their parameter's range", "[dsp][modulation]")
{
    for (const auto* spec : ParameterSpecs::all)
    {
        CAPTURE (spec->id);

        for (const auto value : { spec->min, spec->defaultValue, spec->max })
            for (const auto sourceValue : { 0.0f, 0.5f, 1.0f })
            {
                const auto modulated = ModulateSynth::modulatedValue (*spec, value, { Modulation::Source::LFO, 0, -0.5f, 1.5f }, sourceValue);
                CHECK (modulated >= spec->min);
                CHECK (modulated <= spec->max);
            }
    }
}

TEST_CASE_METHOD (ProcessorFixture, "A waveform changed while a note plays is heard", "[dsp]")
{
    // Morph 0 plays only waveform A, and 1 only B
    const auto morph = GENERATE (0.0f, 1.0f);
    const auto& waveform = morph < 0.5f ? ParameterSpecs::osc[0].waveform : ParameterSpecs::osc[0].waveformB;
    setParameter (processor, ParameterSpecs::osc[0].morph.id, morph);

    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

    const auto blockRms = [&]
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        processor.processBlock (buffer, midi);
        midi.clear();
        return buffer.getRMSLevel (0, 0, blockSize);
    };

    for (int block = 0; block < 20; ++block)
        blockRms();

    const auto sine = blockRms();
    setParameter (processor, waveform.id, 2.0f);
    const auto square = blockRms();

    CAPTURE (sine, square);
    CHECK (square > sine * 1.2f);
}
