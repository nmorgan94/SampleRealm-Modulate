#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>

#include "dsp/ModulateSynth.h"

using Catch::Matchers::WithinAbs;

namespace
{
    struct FakePlayHead final : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override { return position; }

        juce::Optional<PositionInfo> position;
    };
}

TEST_CASE ("A transport has its defaults where the host says nothing", "[transport]")
{
    const auto check = [] (juce::AudioPlayHead* host)
    {
        const auto transport = Transport::from (host);
        CHECK_THAT (transport.bpm, WithinAbs (Transport{}.bpm, 0.0));
        CHECK_FALSE (transport.playing);
    };

    FakePlayHead playHead;
    check (nullptr);
    check (&playHead);
}

TEST_CASE ("A host playing with no position counts as stopped", "[transport]")
{
    juce::AudioPlayHead::PositionInfo position;
    position.setBpm (90.0);
    position.setIsPlaying (true);

    FakePlayHead playHead;
    playHead.position = position;
    const auto withoutPosition = Transport::from (&playHead);
    CHECK_THAT (withoutPosition.bpm, WithinAbs (90.0, 0.0));
    CHECK_FALSE (withoutPosition.playing);

    position.setPpqPosition (8.5);
    playHead.position = position;
    const auto withPosition = Transport::from (&playHead);
    CHECK (withPosition.playing);
    CHECK_THAT (withPosition.ppq, WithinAbs (8.5, 0.0));
}

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
