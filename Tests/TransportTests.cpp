#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>

#include "dsp/ModulateSynth.h"

using Catch::Matchers::WithinAbs;

TEST_CASE ("A synced LFO in Sync mode is at the host's position for the block", "[dsp][lfo][transport]")
{
    constexpr int blockSize = 512;

    const auto synth = std::make_unique<ModulateSynth>();
    ModulateSynth::Settings settings;
    const auto set = [&] (const ParameterSpecs::Spec& spec, float value) { settings.values[ParameterSpecs::indexOf (spec)] = value; };
    set (ParameterSpecs::lfo[0].mode, 1.0f);
    set (ParameterSpecs::lfo[0].sync, 1.0f);
    set (ParameterSpecs::lfo[0].syncRate, 6.0f);   // 1/4, a cycle a beat
    synth->setSettings (settings);
    synth->prepare (48000.0, blockSize, 2);

    juce::AudioBuffer<float> buffer (2, blockSize);
    const juce::MidiBuffer midi;

    synth->process (buffer, midi, { 120.0, true, 3.25 });
    CHECK_THAT (synth->getLive().lfoPhase[0].load(), WithinAbs (0.25, 1.0e-6));

    synth->process (buffer, midi, { 120.0, true, 7.999999999999 });
    CHECK_THAT (synth->getLive().lfoPhase[0].load(), WithinAbs (0.0, 1.0e-6));
}
