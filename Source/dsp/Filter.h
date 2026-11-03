#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <complex>

enum class FilterMode
{
    Lowpass = 0,
    Highpass,
    Bandpass,
    Notch
};

enum class FilterSlope
{
    TwoPole = 0,   // 12 dB/oct
    FourPole       // 24 dB/oct
};

namespace FilterStages
{
    // Pole Qs of a 4th-order Butterworth, split across two biquads.
    inline constexpr float butterworthQ1 = 0.541196f;
    inline constexpr float butterworthQ2 = 1.306563f;
    inline constexpr float neutralQ      = 0.707107f;

    // Two identical cascaded bandpasses narrow the -3 dB bandwidth by this factor,
    // so each stage widens to keep the passband the width the user asked for.
    inline constexpr float bandpassQScale = 0.643594f;

    // A notch is a null, not a rolloff - there is no skirt to steepen.
    inline bool hasSlope(FilterMode mode) { return mode != FilterMode::Notch; }

    inline float stageOneQ(FilterMode mode, float q)
    {
        return (mode == FilterMode::Bandpass) ? q * bandpassQScale : butterworthQ1;
    }

    // Resonance rides on the second stage, scaled so q == neutralQ lands exactly on
    // Butterworth: flat passband, -3 dB at cutoff.
    inline float stageTwoQ(FilterMode mode, float q)
    {
        return (mode == FilterMode::Bandpass) ? q * bandpassQScale
                                              : butterworthQ2 * (q / neutralQ);
    }
}

//==============================================================================
// juce::dsp::IIR's biquad: the same coefficient formulas and transposed direct form II
struct Biquad
{
    struct Coefficients
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

        static Coefficients make(FilterMode mode, double sampleRate, float frequency, float q)
        {
            const auto invQ = 1 / q;

            if (mode == FilterMode::Highpass)
            {
                const auto n = std::tan(juce::MathConstants<float>::pi * frequency / static_cast<float>(sampleRate));
                const auto nSquared = n * n;
                const auto c1 = 1 / (1 + invQ * n + nSquared);
                return { c1, c1 * -2, c1, c1 * 2 * (nSquared - 1), c1 * (1 - invQ * n + nSquared) };
            }

            const auto n = 1 / std::tan(juce::MathConstants<float>::pi * frequency / static_cast<float>(sampleRate));
            const auto nSquared = n * n;

            switch (mode)
            {
                case FilterMode::Bandpass:
                {
                    const auto c1 = 1 / (1 + invQ * n + nSquared);
                    return { c1 * n * invQ, 0, -c1 * n * invQ, c1 * 2 * (1 - nSquared), c1 * (1 - invQ * n + nSquared) };
                }

                case FilterMode::Notch:
                {
                    const auto c1 = 1 / (1 + n * invQ + nSquared);
                    const auto b0 = c1 * (1 + nSquared);
                    const auto b1 = 2 * c1 * (1 - nSquared);
                    return { b0, b1, b0, b1, c1 * (1 - n * invQ + nSquared) };
                }

                case FilterMode::Lowpass:
                case FilterMode::Highpass:
                    break;
            }

            const auto c1 = 1 / (1 + invQ * n + nSquared);
            return { c1, c1 * 2, c1, c1 * 2 * (1 - nSquared), c1 * (1 - invQ * n + nSquared) };
        }

        double getMagnitudeForFrequency(double frequency, double sampleRate) const
        {
            const auto jw = std::exp(std::complex<double>(0.0, -juce::MathConstants<double>::twoPi * frequency / sampleRate));
            const auto numerator = double(b0) + double(b1) * jw + double(b2) * (jw * jw);
            const auto denominator = 1.0 + double(a1) * jw + double(a2) * (jw * jw);
            return std::abs(numerator / denominator);
        }
    };

    float processSample(float sample)
    {
        const auto output = (c.b0 * sample) + s1;
        s1 = (c.b1 * sample) - (c.a1 * output) + s2;
        s2 = (c.b2 * sample) - (c.a2 * output);
        return output;
    }

    void reset() { s1 = s2 = 0.0f; }

    Coefficients c;
    float s1 = 0.0f, s2 = 0.0f;
};

//==============================================================================
class Filter
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        updateCoefficients();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            stage1[ch].reset();
            stage2[ch].reset();
        }
    }

    void setParameters(FilterMode newMode, FilterSlope newSlope, float cutoffHz, float resonanceValue)
    {
        const float newCutoff = juce::jlimit(20.0f, 20000.0f, cutoffHz);
        const float newQ      = 0.5f + juce::jlimit(0.0f, 1.0f, resonanceValue) * 19.5f;

        const bool modeChanged  = (newMode  != mode);
        const bool slopeChanged = (newSlope != slope);

        if (! modeChanged && ! slopeChanged
            && juce::approximatelyEqual(newCutoff, cutoffFrequency)
            && juce::approximatelyEqual(newQ, qFactor))
            return;

        mode            = newMode;
        slope           = newSlope;
        cutoffFrequency = newCutoff;
        qFactor         = newQ;

        if (modeChanged)
        {
            reset();
        }
        else if (slopeChanged)
        {
            for (int ch = 0; ch < 2; ++ch)
                stage2[ch].reset();
        }

        updateCoefficients();
    }

    float processSample(float sample, int channel)
    {
        int ch = juce::jlimit(0, 1, channel);

        const float first = stage1[ch].processSample(sample);

        return fourPoleActive ? stage2[ch].processSample(first) : first;
    }

    float getMagnitudeAt(float frequency) const
    {
        if (sampleRate <= 0.0)
            return 1.0f;

        auto magnitude = static_cast<float>(stage1[0].c.getMagnitudeForFrequency(frequency, sampleRate));

        if (fourPoleActive)
            magnitude *= static_cast<float>(stage2[0].c.getMagnitudeForFrequency(frequency, sampleRate));

        return magnitude;
    }

    FilterMode getMode() const { return mode; }
    FilterSlope getSlope() const { return slope; }

private:
    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;

        fourPoleActive = (slope == FilterSlope::FourPole) && FilterStages::hasSlope(mode);

        // Two-pole runs the user's Q directly; four-pole splits it across the stages.
        const float q1 = fourPoleActive ? FilterStages::stageOneQ(mode, qFactor) : qFactor;

        const auto first = Biquad::Coefficients::make(mode, sampleRate, cutoffFrequency, q1);
        for (int ch = 0; ch < 2; ++ch)
            stage1[ch].c = first;

        if (! fourPoleActive)
            return;

        const auto second = Biquad::Coefficients::make(mode, sampleRate, cutoffFrequency,
                                                       FilterStages::stageTwoQ(mode, qFactor));
        for (int ch = 0; ch < 2; ++ch)
            stage2[ch].c = second;
    }

    FilterMode  mode  = FilterMode::Lowpass;
    FilterSlope slope = FilterSlope::TwoPole;
    double sampleRate = 44100.0;
    float cutoffFrequency = 1000.0f;
    float qFactor = 0.707f;
    bool fourPoleActive = false;

    Biquad stage1[2];
    Biquad stage2[2];
};
