#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "ui/MeterBallistics.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

//==============================================================================
TEST_CASE ("Peak meter holds each channel's loudest sample until read", "[meter]")
{
    PeakMeter meter { 2 };
    auto buffer = makeConstantBuffer (2, 64, 0.0f);

    buffer.setSample (0, 10, -0.8f);    // a negative peak counts as much as a positive one
    buffer.setSample (1, 3, 0.3f);
    meter.process (buffer);

    // A quieter block afterwards doesn't lower what's waiting to be read
    meter.process (makeConstantBuffer (2, 64, 0.1f));

    CHECK_THAT (meter.readAndReset (0), WithinAbs (0.8, 1.0e-6));
    CHECK_THAT (meter.readAndReset (1), WithinAbs (0.3, 1.0e-6));

    CHECK_THAT (meter.readAndReset (0), WithinAbs (0.0, 0.0));
    CHECK_THAT (meter.readAndReset (1), WithinAbs (0.0, 0.0));
}

TEST_CASE ("A mono peak meter ignores extra channels", "[meter]")
{
    PeakMeter meter { 2 };
    meter.setNumChannels (1);
    REQUIRE (meter.getNumChannels() == 1);

    auto buffer = makeConstantBuffer (2, 64, 0.2f);
    buffer.applyGain (1, 0, 64, 4.0f);
    meter.process (buffer);

    CHECK_THAT (meter.readAndReset (0), WithinAbs (0.2, 1.0e-6));
    CHECK_THAT (meter.readAndReset (1), WithinAbs (0.0, 0.0));
}

TEST_CASE ("A channel dropped from metering reads silent", "[meter]")
{
    PeakMeter meter { 2 };
    meter.process (makeConstantBuffer (2, 64, 0.5f));
    meter.setNumChannels (1);

    CHECK_THAT (meter.readAndReset (1), WithinAbs (0.0, 0.0));
}

TEST_CASE_METHOD (ProcessorFixture, "The output meter reads the synth's output", "[meter]")
{
    processor.getOutputMeter().reset();

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    processor.processBlock (buffer, midi);

    for (int channel = 0; channel < 2; ++channel)
    {
        CAPTURE (channel);
        const auto peak = buffer.getMagnitude (channel, 0, blockSize);
        REQUIRE (peak > 0.0f);
        CHECK_THAT (processor.getOutputMeter().readAndReset (channel), WithinAbs (peak, 1.0e-6));
    }
}

//==============================================================================
TEST_CASE ("Meter bars rise at once and fall smoothly", "[meter][ui]")
{
    MeterBallistics bar;

    CHECK_THAT (bar.levelDb, WithinAbs (MeterBallistics::floorDb, 0.0));

    CHECK (bar.update (0.5f));
    CHECK_THAT (bar.levelDb, WithinAbs (-6.02, 0.01));

    CHECK (bar.update (1.0f));
    CHECK_THAT (bar.levelDb, WithinAbs (0.0, 1.0e-6));
    CHECK_THAT (bar.fraction(), WithinAbs (1.0, 1.0e-6));

    // One second of silence falls 20 dB
    for (int tick = 0; tick < MeterBallistics::updateHz; ++tick)
        bar.update (0.0f);

    CHECK_THAT (bar.levelDb, WithinAbs (-20.0, 0.01));

    // Then it settles on the floor and stops asking for repaints
    for (int tick = 0; tick < 10 * MeterBallistics::updateHz; ++tick)
        bar.update (0.0f);

    CHECK_THAT (bar.levelDb, WithinAbs (MeterBallistics::floorDb, 0.0));
    CHECK_THAT (bar.fraction(), WithinAbs (0.0, 0.0));
    CHECK_FALSE (bar.update (0.0f));
}

TEST_CASE ("Meter clip light holds for two seconds after going over 0 dB", "[meter][ui]")
{
    MeterBallistics bar;

    const auto peak = GENERATE (1.5f, std::numeric_limits<float>::infinity());
    CAPTURE (peak);

    CHECK_FALSE (bar.isClipping());
    bar.update (peak);

    CHECK (bar.isClipping());
    CHECK_THAT (bar.levelDb, WithinAbs (0.0, 0.0));    // capped at the top of the scale

    for (int tick = 1; tick < 2 * MeterBallistics::updateHz; ++tick)
        bar.update (0.0f);

    CHECK (bar.isClipping());
    CHECK (bar.levelDb < 0.0f);     // even an infinite peak falls away

    CHECK (bar.update (0.0f));      // the light going out needs a repaint
    CHECK_FALSE (bar.isClipping());
}
