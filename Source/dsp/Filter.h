#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>

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
        const float newCutoff = juce::jlimit(20.0f, juce::jmin(20000.0f, 0.49f * static_cast<float>(sampleRate)), cutoffHz);
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

    void process(juce::dsp::AudioBlock<float> block)
    {
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            const auto ch = std::min<size_t>(channel, 1);
            auto channelBlock = block.getSingleChannelBlock(channel);
            const juce::dsp::ProcessContextReplacing<float> context(channelBlock);

            stage1[ch].process(context);

            if (fourPoleActive)
                stage2[ch].process(context);
        }
    }

    float getMagnitudeAt(float frequency) const
    {
        if (sampleRate <= 0.0)
            return 1.0f;

        const auto underNyquist = juce::jmin<double>(frequency, 0.5 * sampleRate);
        auto magnitude = static_cast<float>(stage1[0].coefficients->getMagnitudeForFrequency(underNyquist, sampleRate));

        if (fourPoleActive)
            magnitude *= static_cast<float>(stage2[0].coefficients->getMagnitudeForFrequency(underNyquist, sampleRate));

        return magnitude;
    }

    FilterMode getMode() const { return mode; }
    FilterSlope getSlope() const { return slope; }

private:
    static std::array<float, 6> makeCoefficients(FilterMode filterMode, double sr, float freq, float q)
    {
        using Coefficients = juce::dsp::IIR::ArrayCoefficients<float>;

        switch (filterMode)
        {
            case FilterMode::Highpass:  return Coefficients::makeHighPass(sr, freq, q);
            case FilterMode::Bandpass:  return Coefficients::makeBandPass(sr, freq, q);
            case FilterMode::Notch:     return Coefficients::makeNotch(sr, freq, q);
            case FilterMode::Lowpass:   break;
        }

        return Coefficients::makeLowPass(sr, freq, q);
    }

    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;

        fourPoleActive = (slope == FilterSlope::FourPole) && FilterStages::hasSlope(mode);

        // Two-pole runs the user's Q directly; four-pole splits it across the stages.
        const float q1 = fourPoleActive ? FilterStages::stageOneQ(mode, qFactor) : qFactor;

        const auto first = makeCoefficients(mode, sampleRate, cutoffFrequency, q1);
        for (int ch = 0; ch < 2; ++ch)
            *stage1[ch].coefficients = first;

        // Even unused, so the audio thread never changes its order, which allocates
        const auto second = makeCoefficients(mode, sampleRate, cutoffFrequency,
                                             FilterStages::stageTwoQ(mode, qFactor));
        for (int ch = 0; ch < 2; ++ch)
            *stage2[ch].coefficients = second;
    }

    FilterMode  mode  = FilterMode::Lowpass;
    FilterSlope slope = FilterSlope::TwoPole;
    double sampleRate = 44100.0;
    float cutoffFrequency = 1000.0f;
    float qFactor = 0.707f;
    bool fourPoleActive = false;

    juce::dsp::IIR::Filter<float> stage1[2];
    juce::dsp::IIR::Filter<float> stage2[2];
};
