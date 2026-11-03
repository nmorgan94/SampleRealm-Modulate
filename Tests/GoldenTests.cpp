#include <catch2/catch_test_macros.hpp>

#include <span>

#include "TestHelpers.h"

using namespace TestHelpers;

namespace
{
    // Shared with wasm/engine.test.mjs, which checks the engine against the same fingerprints
    const auto goldenFile = juce::File (__FILE__).getSiblingFile ("fixtures/golden.json");

    using Fingerprint = std::vector<std::array<float, 4>>;

    // RMS then peak, left then right, for each frame of blocksPerFrame blocks
    Fingerprint render (const juce::var& golden, const juce::var& scenario)
    {
        const auto sampleRate = static_cast<double> (golden["sampleRate"]);
        const int blockSize = golden["blockSize"];
        const int blocksPerFrame = golden["blocksPerFrame"];

        juce::ScopedJuceInitialiser_GUI juceInitialiser;
        AudioPluginAudioProcessor processor;
        processor.setRateAndBufferSizeDetails (sampleRate, blockSize);

        for (const auto& [name, value] : scenario["params"].getDynamicObject()->getProperties())
            setParameter (processor, name.toString(), value);

        for (const auto& modulation : *scenario["modulation"].getArray())
        {
            const auto source = modulation["source"] == "lfo" ? ModulationManager::ModulationSource::LFO
                                                               : ModulationManager::ModulationSource::Envelope;
            processor.getModulationManager().setAssignment (modulation["param"].toString(),
                { source, modulation["index"], modulation["min"], modulation["max"] });
        }

        for (const auto& curve : *scenario["curves"].getArray())
        {
            std::vector<LfoCurve::Point> points;

            for (const auto& point : *curve["points"].getArray())
                points.emplace_back (point[0], point[1]);

            const int lfo = curve["lfo"];
            processor.setLFOCurvePoints (static_cast<size_t> (lfo), points);
        }

        // Builds the LFO tables from the curves and tensions above
        processor.prepareToPlay (sampleRate, blockSize);

        Fingerprint fingerprint;
        std::array<double, 2> sumOfSquares {};
        std::array<float, 2> peaks {};
        const int numBlocks = scenario["blocks"];

        for (int block = 0; block < numBlocks; ++block)
        {
            juce::AudioBuffer<float> buffer (2, blockSize);
            juce::MidiBuffer midi;

            for (const auto& note : *scenario["notes"].getArray())
            {
                if (static_cast<int> (note["block"]) != block)
                    continue;

                const int velocity = note["velocity"];
                midi.addEvent (velocity > 0 ? juce::MidiMessage::noteOn (1, note["note"], static_cast<juce::uint8> (velocity))
                                            : juce::MidiMessage::noteOff (1, note["note"]),
                               0);
            }

            processor.processBlock (buffer, midi);

            for (size_t channel = 0; channel < 2; ++channel)
            {
                for (const auto sample : std::span (buffer.getReadPointer (static_cast<int> (channel)), static_cast<size_t> (blockSize)))
                {
                    sumOfSquares[channel] += sample * sample;
                    peaks[channel] = std::max (peaks[channel], std::abs (sample));
                }
            }

            if ((block + 1) % blocksPerFrame == 0)
            {
                const auto rms = [&] (size_t channel)
                {
                    return static_cast<float> (std::sqrt (sumOfSquares[channel] / (blockSize * blocksPerFrame)));
                };

                fingerprint.push_back ({ rms (0), rms (1), peaks[0], peaks[1] });
                sumOfSquares = {};
                peaks = {};
            }
        }

        processor.releaseResources();
        return fingerprint;
    }

    // A line per frame, note and assignment, so a re-record's diff shows what moved
    juce::String toGoldenText (const juce::var& golden)
    {
        const auto options = juce::JSON::FormatOptions{}.withSpacing (juce::JSON::Spacing::singleLine).withMaxDecimalPlaces (7);
        const auto line = [&] (const juce::var& value) { return juce::JSON::toString (value, options); };

        juce::StringArray scenarios;

        for (const auto& scenario : *golden["scenarios"].getArray())
        {
            juce::StringArray properties;

            for (const auto& [name, value] : scenario.getDynamicObject()->getProperties())
            {
                auto text = "      " + line (name.toString()) + ": ";
                const auto* elements = value.getArray();

                if (elements == nullptr || elements->isEmpty())
                {
                    properties.add (text + line (value));
                    continue;
                }

                juce::StringArray lines;

                for (const auto& element : *elements)
                    lines.add ("        " + line (element));

                properties.add (text + "[\n" + lines.joinIntoString (",\n") + "\n      ]");
            }

            scenarios.add ("    {\n" + properties.joinIntoString (",\n") + "\n    }");
        }

        juce::StringArray header;

        for (const auto& [name, value] : golden.getDynamicObject()->getProperties())
            if (name.toString() != "scenarios")
                header.add ("  " + line (name.toString()) + ": " + line (value));

        return "{\n" + header.joinIntoString (",\n") + ",\n  \"scenarios\": [\n" + scenarios.joinIntoString (",\n") + "\n  ]\n}\n";
    }

    juce::var readGolden()
    {
        auto golden = juce::JSON::parse (goldenFile);
        REQUIRE (golden.isObject());
        return golden;
    }
}

//==============================================================================
TEST_CASE ("Renders match the golden fingerprints", "[dsp][golden]")
{
    const auto golden = readGolden();

    for (const auto& scenario : *golden["scenarios"].getArray())
    {
        CAPTURE (scenario["name"].toString());

        const auto* expected = scenario["fingerprint"].getArray();
        REQUIRE (expected != nullptr);

        const auto actual = render (golden, scenario);
        REQUIRE (actual.size() == static_cast<size_t> (expected->size()));

        int firstMismatch = -1;

        for (size_t frame = 0; frame < actual.size() && firstMismatch < 0; ++frame)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                const auto want = static_cast<float> ((*expected)[static_cast<int> (frame)][static_cast<int> (i)]);

                if (std::abs (actual[frame][i] - want) > 1.0e-5f + 1.0e-4f * std::abs (want))
                    firstMismatch = static_cast<int> (frame);
            }
        }

        CAPTURE (firstMismatch);
        CHECK (firstMismatch == -1);
    }
}

// Run only to record new fingerprints: Modulate_Tests "[.write-golden]"
TEST_CASE ("Write the golden fingerprints", "[.write-golden]")
{
    auto golden = readGolden();

    for (auto& scenario : *golden["scenarios"].getArray())
    {
        juce::Array<juce::var> frames;

        for (const auto& frame : render (golden, scenario))
            frames.add (juce::Array<juce::var> { frame[0], frame[1], frame[2], frame[3] });

        scenario.getDynamicObject()->setProperty ("fingerprint", frames);
    }

    REQUIRE (goldenFile.replaceWithText (toGoldenText (golden)));
}
