#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <complex>
#include <span>
#include <vector>

#include "dsp/ModulateSynth.h"

namespace
{
    // Amplitude of each bin, scaled so a full-scale sine reads 1
    std::vector<double> magnitudes(std::span<const float> samples)
    {
        const std::vector<std::complex<float>> input(samples.begin(), samples.end());
        std::vector<std::complex<float>> spectrum(samples.size());

        const juce::dsp::FFT fft(juce::findHighestSetBit(static_cast<juce::uint32>(samples.size())));
        fft.perform(input.data(), spectrum.data(), false);

        std::vector<double> result;
        for (const auto& bin : std::span(spectrum).first(spectrum.size() / 2 + 1))
            result.push_back(2.0 * std::abs(bin) / static_cast<double>(samples.size()));

        return result;
    }

    std::span<const float> channel(const juce::AudioBuffer<float>& buffer, int index)
    {
        return { buffer.getReadPointer(index), static_cast<size_t>(buffer.getNumSamples()) };
    }
}

//==============================================================================
TEST_CASE("Each bank level keeps only its own harmonics", "[dsp][wavetable]")
{
    const auto type = GENERATE(WaveformType::Saw, WaveformType::Square, WaveformType::Pulse10, WaveformType::BitcrushedRamp);
    CAPTURE(static_cast<int>(type));

    juce::AudioBuffer<float> naive(1, WavetableBank::size);
    WavetableGenerator::generateWavetable(naive, type);
    const auto naiveMagnitudes = magnitudes(channel(naive, 0));

    const auto& levels = WavetableBank::get(type);
    REQUIRE(levels.getNumChannels() == WavetableBank::numLevels);

    for (int level = 0; level < WavetableBank::numLevels; ++level)
    {
        CAPTURE(level);
        const auto kept = static_cast<size_t>(WavetableBank::maxHarmonics >> level);
        const auto levelMagnitudes = magnitudes(channel(levels, level));

        CHECK(levelMagnitudes[0] < 1.0e-5);

        double worstAbove = 0.0, worstBelow = 0.0;

        for (size_t harmonic = 1; harmonic < levelMagnitudes.size(); ++harmonic)
        {
            if (harmonic > kept)
                worstAbove = std::max(worstAbove, levelMagnitudes[harmonic]);
            else
                worstBelow = std::max(worstBelow, std::abs(levelMagnitudes[harmonic] - naiveMagnitudes[harmonic]));
        }

        CHECK(worstAbove < 1.0e-5);
        CHECK(worstBelow < 1.0e-5);
    }
}

TEST_CASE("The chosen level has the most harmonics that fit under the limit", "[dsp][wavetable]")
{
    const auto sampleRate = GENERATE(44100.0, 48000.0, 96000.0);
    CAPTURE(sampleRate);

    const auto limitHz = std::max(0.45 * sampleRate, sampleRate - 20000.0);

    for (int note = 0; note <= 127 + 72; ++note)
    {
        CAPTURE(note);
        const auto frequency = juce::MidiMessage::getMidiNoteInHertz(note);
        const auto level = WavetableBank::levelFor(frequency, sampleRate);

        if (frequency > limitHz)
        {
            CHECK(level == -1);
            continue;
        }

        REQUIRE(level >= 0);
        REQUIRE(level < WavetableBank::numLevels);
        CHECK((WavetableBank::maxHarmonics >> level) * frequency <= limitHz);

        if (level > 0)
            CHECK((WavetableBank::maxHarmonics >> (level - 1)) * frequency > limitHz);
    }
}

TEST_CASE("A high saw has nothing between its harmonics", "[dsp][wavetable]")
{
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int note = 100;
    const auto frequency = juce::MidiMessage::getMidiNoteInHertz(note);

    const auto synth = std::make_unique<ModulateSynth>();
    ModulateSynth::Settings settings;
    settings.values[ParameterSpecs::indexOf(ParameterSpecs::osc[0].waveform)] = static_cast<float>(WaveformType::Saw);
    settings.values[ParameterSpecs::indexOf(ParameterSpecs::osc[0].waveformB)] = static_cast<float>(WaveformType::Saw);
    settings.values[ParameterSpecs::indexOf(ParameterSpecs::filterEnable)] = 0.0f;
    synth->setSettings(settings);
    synth->prepare(sampleRate, blockSize, 2);

    // Past the attack and decay, to the sustain level
    constexpr int numBlocks = 40, analysedBlocks = 16;
    juce::AudioBuffer<float> recorded(1, analysedBlocks * blockSize);

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::MidiBuffer midi;

        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, note, 1.0f), 0);

        synth->process(buffer, midi);

        if (const auto start = block - (numBlocks - analysedBlocks); start >= 0)
            recorded.copyFrom(0, start * blockSize, buffer, 0, 0, blockSize);
    }

    // Blackman-Harris, so each harmonic's leakage stays within a few bins
    const auto length = static_cast<size_t>(recorded.getNumSamples());
    juce::dsp::WindowingFunction<float>(length, juce::dsp::WindowingFunction<float>::blackmanHarris, false)
        .multiplyWithWindowingTable(recorded.getWritePointer(0), length);

    const auto spectrum = magnitudes(channel(recorded, 0));
    const auto binHz = sampleRate / static_cast<double>(length);

    double fundamental = 0.0, loudestAlias = 0.0;

    for (size_t bin = 1; bin < spectrum.size() && static_cast<double>(bin) * binHz < 20000.0; ++bin)
    {
        const auto hz = static_cast<double>(bin) * binHz;
        const auto harmonic = std::round(hz / frequency);
        const bool nearHarmonic = harmonic >= 1.0 && std::abs(hz - harmonic * frequency) <= 4.0 * binHz;

        if (! nearHarmonic)
            loudestAlias = std::max(loudestAlias, spectrum[bin]);
        else if (juce::exactlyEqual(harmonic, 1.0))
            fundamental = std::max(fundamental, spectrum[bin]);
    }

    REQUIRE(fundamental > 0.0);
    const auto aliasDb = juce::Decibels::gainToDecibels(loudestAlias / fundamental, -200.0);
    CAPTURE(aliasDb);
    CHECK(aliasDb < -70.0);
}
