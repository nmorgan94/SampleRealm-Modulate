#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <complex>
#include <span>
#include <vector>
#include "../shared/ParameterSpecs.h"
#include "WavetableGenerator.h"

//==============================================================================
// Every waveform, band-limited once per octave: channel L keeps harmonics 1 to maxHarmonics >> L
class WavetableBank
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int size = WavetableGenerator::getWavetableSize();
    static_assert(size == 1 << fftOrder);
    static constexpr int maxHarmonics = size / 2;
    static constexpr int numLevels = fftOrder;

    // Builds every waveform on the first call, so make that one off the audio thread
    static const juce::AudioBuffer<float>& get(WaveformType type)
    {
        static const auto bank = build();
        return bank[static_cast<size_t>(type)];
    }

    // The level with the most harmonics that all stay under the limit, or -1 when the fundamental doesn't
    static int levelFor(double frequencyHz, double sampleRate)
    {
        // Harmonics above Nyquist fold back to 20 kHz or higher
        const auto limitHz = juce::jmax(0.45 * sampleRate, sampleRate - 20000.0);
        const auto harmonicsUnderLimit = limitHz / frequencyHz;

        if (harmonicsUnderLimit < 1.0)
            return -1;

        int level = 0;
        while ((maxHarmonics >> level) > harmonicsUnderLimit)
            ++level;

        return level;
    }

private:
    static std::vector<juce::AudioBuffer<float>> build()
    {
        constexpr auto length = static_cast<size_t>(size);

        std::vector<juce::AudioBuffer<float>> bank;
        bank.reserve(ParameterSpecs::waveformNames.size());

        const juce::dsp::FFT fft(fftOrder);
        juce::AudioBuffer<float> naive(1, size);
        std::vector<std::complex<float>> samples(length), spectrum(length), levelSpectrum(length);

        for (size_t type = 0; type < ParameterSpecs::waveformNames.size(); ++type)
        {
            WavetableGenerator::generateWavetable(naive, static_cast<WaveformType>(type));
            std::ranges::copy(std::span(naive.getReadPointer(0), length), samples.begin());
            fft.perform(samples.data(), spectrum.data(), false);

            auto& levels = bank.emplace_back(numLevels, size);

            for (int level = 0; level < numLevels; ++level)
            {
                const auto kept = static_cast<size_t>(maxHarmonics >> level);

                for (size_t bin = 0; bin < length; ++bin)
                {
                    const auto harmonic = std::min(bin, length - bin);
                    levelSpectrum[bin] = harmonic >= 1 && harmonic <= kept ? spectrum[bin] : std::complex<float>{};
                }

                fft.perform(levelSpectrum.data(), samples.data(), true);

                std::ranges::transform(samples, levels.getWritePointer(level),
                                       [](const auto& value) { return value.real(); });
            }
        }

        return bank;
    }
};
