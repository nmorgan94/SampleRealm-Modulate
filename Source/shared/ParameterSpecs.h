#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <span>

//==============================================================================
namespace ParameterSpecs
{
    enum class Kind { real, integer, choice };

    struct Spec
    {
        const char* id;
        const char* name;
        Kind kind;
        float min, max, defaultValue;
        float step = 0.0f;
        float skew = 1.0f;
        const char* unit = "";
        std::span<const char* const> choices = {};
    };

    constexpr Spec integer (const char* id, const char* name, int min, int max, int defaultValue, const char* unit = "")
    {
        return { id, name, Kind::integer, static_cast<float> (min), static_cast<float> (max),
                 static_cast<float> (defaultValue), 1.0f, 1.0f, unit };
    }

    constexpr Spec choice (const char* id, const char* name, std::span<const char* const> choices, int defaultValue)
    {
        return { id, name, Kind::choice, 0.0f, static_cast<float> (choices.size() - 1),
                 static_cast<float> (defaultValue), 1.0f, 1.0f, "", choices };
    }

    inline constexpr int maxVoices = 8;
    inline constexpr int maxUnison = 7;

    inline constexpr std::array offOn { "Off", "On" };

    inline constexpr std::array waveformNames {
        "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%", "Formant Sine", "Formant Vowel",
        "Bitcrushed Ramp", "Fm Operator", "Feedback Fm", "Wavefolded Sine", "Fractal Pulse", "Double Saw",
        "Resonant Saw", "Prime Cluster", "Odd Harmonic Cluster", "Log Square", "Asymmetric Rectified Sine",
        "Interleaved Saw Pulse", "Trapezoid"
    };

    inline constexpr std::array noiseTypes { "White", "Pink" };
    inline constexpr std::array lfoModes { "Trigger", "Sync" };

    inline constexpr std::array syncRates {
        "1/16", "1/16T", "1/16D", "1/8", "1/8T", "1/8D",
        "1/4", "1/4T", "1/4D", "1/2", "1/2T", "1/2D",
        "1 Bar", "2 Bars", "4 Bars"
    };

    inline constexpr std::array filterModes { "Lowpass", "Highpass", "Bandpass", "Notch" };
    inline constexpr std::array filterSlopes { "12 dB", "24 dB" };
    inline constexpr std::array glideModes { "Time", "Rate" };

    //==============================================================================
    inline constexpr Spec masterGain { "master_gain", "Master Gain", Kind::real, -24.0f, 6.0f, 0.0f, 0.1f };

    struct Oscillator { Spec enable, waveform, waveformB, morph, gain, pitch, fine; };

    inline constexpr std::array osc {
        Oscillator {
            choice ("osc1_enable", "Osc 1 Enable", offOn, 1),
            choice ("osc1_waveform", "Osc 1 Waveform", waveformNames, 0),
            choice ("osc1_waveform_b", "Osc 1 Waveform B", waveformNames, 0),
            { "osc1_morph", "Osc 1 Morph", Kind::real, 0.0f, 1.0f, 0.0f, 0.01f },
            { "osc1_gain", "Osc 1 Gain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            integer ("osc1_pitch", "Osc 1 Pitch", -36, 36, 0),
            integer ("osc1_fine", "Osc 1 Fine", -99, 99, 0)
        },
        Oscillator {
            choice ("osc2_enable", "Osc 2 Enable", offOn, 0),
            choice ("osc2_waveform", "Osc 2 Waveform", waveformNames, 1),
            choice ("osc2_waveform_b", "Osc 2 Waveform B", waveformNames, 1),
            { "osc2_morph", "Osc 2 Morph", Kind::real, 0.0f, 1.0f, 0.0f, 0.01f },
            { "osc2_gain", "Osc 2 Gain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            integer ("osc2_pitch", "Osc 2 Pitch", -36, 36, 0),
            integer ("osc2_fine", "Osc 2 Fine", -99, 99, 0)
        },
        Oscillator {
            choice ("osc3_enable", "Osc 3 Enable", offOn, 0),
            choice ("osc3_waveform", "Osc 3 Waveform", waveformNames, 2),
            choice ("osc3_waveform_b", "Osc 3 Waveform B", waveformNames, 2),
            { "osc3_morph", "Osc 3 Morph", Kind::real, 0.0f, 1.0f, 0.0f, 0.01f },
            { "osc3_gain", "Osc 3 Gain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            integer ("osc3_pitch", "Osc 3 Pitch", -36, 36, 0),
            integer ("osc3_fine", "Osc 3 Fine", -99, 99, 0)
        }
    };

    inline constexpr Spec noiseEnable = choice ("noise_enable", "Noise Enable", offOn, 0);
    inline constexpr Spec noiseType = choice ("noise_type", "Noise Type", noiseTypes, 0);
    inline constexpr Spec noiseLevel { "noise_level", "Noise Level", Kind::real, 0.0f, 1.0f, 0.5f, 0.01f };

    struct Lfo { Spec mode, rate, sync, syncRate, tension; };

    inline constexpr std::array lfo {
        Lfo {
            choice ("lfo1_mode", "LFO 1 Mode", lfoModes, 0),
            { "lfo1_rate", "LFO 1 Rate", Kind::real, 0.01f, 20.0f, 1.0f, 0.01f, 0.3f },
            choice ("lfo1_sync", "LFO 1 Sync", offOn, 0),
            choice ("lfo1_sync_rate", "LFO 1 Sync Rate", syncRates, 6),
            { "lfo1_tension", "LFO 1 Tension", Kind::real, 0.0f, 1.0f, 0.5f, 0.01f }
        },
        Lfo {
            choice ("lfo2_mode", "LFO 2 Mode", lfoModes, 0),
            { "lfo2_rate", "LFO 2 Rate", Kind::real, 0.01f, 20.0f, 2.0f, 0.01f, 0.3f },
            choice ("lfo2_sync", "LFO 2 Sync", offOn, 0),
            choice ("lfo2_sync_rate", "LFO 2 Sync Rate", syncRates, 6),
            { "lfo2_tension", "LFO 2 Tension", Kind::real, 0.0f, 1.0f, 0.5f, 0.01f }
        },
        Lfo {
            choice ("lfo3_mode", "LFO 3 Mode", lfoModes, 0),
            { "lfo3_rate", "LFO 3 Rate", Kind::real, 0.01f, 20.0f, 4.0f, 0.01f, 0.3f },
            choice ("lfo3_sync", "LFO 3 Sync", offOn, 0),
            choice ("lfo3_sync_rate", "LFO 3 Sync Rate", syncRates, 6),
            { "lfo3_tension", "LFO 3 Tension", Kind::real, 0.0f, 1.0f, 0.5f, 0.01f }
        },
        Lfo {
            choice ("lfo4_mode", "LFO 4 Mode", lfoModes, 0),
            { "lfo4_rate", "LFO 4 Rate", Kind::real, 0.01f, 20.0f, 8.0f, 0.01f, 0.3f },
            choice ("lfo4_sync", "LFO 4 Sync", offOn, 0),
            choice ("lfo4_sync_rate", "LFO 4 Sync Rate", syncRates, 6),
            { "lfo4_tension", "LFO 4 Tension", Kind::real, 0.0f, 1.0f, 0.5f, 0.01f }
        }
    };

    inline constexpr Spec filterEnable = choice ("filter_enable", "Filter Enable", offOn, 1);
    inline constexpr Spec filterMode = choice ("filter_mode", "Filter Mode", filterModes, 0);
    inline constexpr Spec filterSlope = choice ("filter_slope", "Filter Slope", filterSlopes, 0);
    inline constexpr Spec filterCutoff { "filter_cutoff", "Filter Cutoff", Kind::real, 20.0f, 20000.0f, 1000.0f, 1.0f, 0.3f };
    inline constexpr Spec filterResonance { "filter_resonance", "Filter Resonance", Kind::real, 0.0f, 1.0f, 0.5f, 0.01f };
    inline constexpr Spec drive { "drive", "Drive", Kind::real, 0.0f, 1.0f, 0.0f, 0.01f };
    inline constexpr Spec driveHq = choice ("drive_hq", "Drive HQ", offOn, 0);

    inline constexpr Spec glideTime { "glide_time", "Glide Time", Kind::real, 0.0f, 2.0f, 0.0f, 0.001f, 0.4f };
    inline constexpr Spec glideMode = choice ("glide_mode", "Glide Mode", glideModes, 1);
    inline constexpr Spec coarsePitch { "coarse_pitch", "Coarse Pitch", Kind::real, -36.0f, 36.0f, 0.0f, 0.01f };
    inline constexpr Spec numVoices = integer ("num_voices", "Voices", 1, maxVoices, maxVoices);
    inline constexpr Spec unisonVoices = integer ("unison_voices", "Unison", 1, maxUnison, 1);
    inline constexpr Spec unisonDetune = integer ("unison_detune", "Detune", 0, 100, 15);
    inline constexpr Spec unisonSpread = integer ("unison_spread", "Spread", 0, 100, 50);

    struct Envelope { Spec attack, decay, sustain, release; };

    inline constexpr std::array env {
        Envelope {
            { "env1_attack", "Env 1 Attack", Kind::real, 0.001f, 5.0f, 0.01f, 0.001f, 0.3f },
            { "env1_decay", "Env 1 Decay", Kind::real, 0.001f, 5.0f, 0.1f, 0.001f, 0.3f },
            { "env1_sustain", "Env 1 Sustain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            { "env1_release", "Env 1 Release", Kind::real, 0.001f, 5.0f, 0.3f, 0.001f, 0.3f }
        },
        Envelope {
            { "env2_attack", "Env 2 Attack", Kind::real, 0.001f, 5.0f, 0.01f, 0.001f, 0.3f },
            { "env2_decay", "Env 2 Decay", Kind::real, 0.001f, 5.0f, 0.1f, 0.001f, 0.3f },
            { "env2_sustain", "Env 2 Sustain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            { "env2_release", "Env 2 Release", Kind::real, 0.001f, 5.0f, 0.3f, 0.001f, 0.3f }
        },
        Envelope {
            { "env3_attack", "Env 3 Attack", Kind::real, 0.001f, 5.0f, 0.01f, 0.001f, 0.3f },
            { "env3_decay", "Env 3 Decay", Kind::real, 0.001f, 5.0f, 0.1f, 0.001f, 0.3f },
            { "env3_sustain", "Env 3 Sustain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            { "env3_release", "Env 3 Release", Kind::real, 0.001f, 5.0f, 0.3f, 0.001f, 0.3f }
        },
        Envelope {
            { "env4_attack", "Env 4 Attack", Kind::real, 0.001f, 5.0f, 0.01f, 0.001f, 0.3f },
            { "env4_decay", "Env 4 Decay", Kind::real, 0.001f, 5.0f, 0.1f, 0.001f, 0.3f },
            { "env4_sustain", "Env 4 Sustain", Kind::real, 0.0f, 1.0f, 0.7f, 0.01f },
            { "env4_release", "Env 4 Release", Kind::real, 0.001f, 5.0f, 0.3f, 0.001f, 0.3f }
        }
    };

    // The host's parameter order
    inline constexpr std::array all {
        &masterGain,
        &osc[0].enable, &osc[0].waveform, &osc[0].waveformB, &osc[0].morph, &osc[0].gain, &osc[0].pitch, &osc[0].fine,
        &osc[1].enable, &osc[1].waveform, &osc[1].waveformB, &osc[1].morph, &osc[1].gain, &osc[1].pitch, &osc[1].fine,
        &osc[2].enable, &osc[2].waveform, &osc[2].waveformB, &osc[2].morph, &osc[2].gain, &osc[2].pitch, &osc[2].fine,
        &noiseEnable, &noiseType, &noiseLevel,
        &lfo[0].mode, &lfo[0].rate, &lfo[0].sync, &lfo[0].syncRate, &lfo[0].tension,
        &lfo[1].mode, &lfo[1].rate, &lfo[1].sync, &lfo[1].syncRate, &lfo[1].tension,
        &lfo[2].mode, &lfo[2].rate, &lfo[2].sync, &lfo[2].syncRate, &lfo[2].tension,
        &lfo[3].mode, &lfo[3].rate, &lfo[3].sync, &lfo[3].syncRate, &lfo[3].tension,
        &filterEnable, &filterMode, &filterSlope, &filterCutoff, &filterResonance, &drive, &driveHq,
        &glideTime, &glideMode, &coarsePitch, &numVoices, &unisonVoices, &unisonDetune, &unisonSpread,
        &env[0].attack, &env[0].decay, &env[0].sustain, &env[0].release,
        &env[1].attack, &env[1].decay, &env[1].sustain, &env[1].release,
        &env[2].attack, &env[2].decay, &env[2].sustain, &env[2].release,
        &env[3].attack, &env[3].decay, &env[3].sustain, &env[3].release
    };

    static_assert (std::ranges::all_of (all, [] (const Spec* spec)
    {
        if (spec->kind == Kind::real)
            return true;

        return spec->step == 1.0f
            && (spec->kind == Kind::integer
                || (spec->min == 0.0f && static_cast<size_t> (spec->max) == spec->choices.size() - 1));
    }), "Integer and choice specs need step 1, and a choice spec's range is 0 to its last choice");

    constexpr size_t indexOf (const Spec& spec)
    {
        return static_cast<size_t> (std::ranges::find (all, &spec) - all.begin());
    }

    //==============================================================================
    // The ranges JUCE gives AudioParameterFloat, Int and Choice, so the DSP sees the values the host does

    inline float snapToLegalValue (const Spec& spec, float value)
    {
        if (spec.kind != Kind::real)
            return std::nearbyint (std::clamp (value, spec.min, spec.max)); // roundToInt rounds halves to even

        if (spec.step > 0.0f)
            value = spec.min + spec.step * std::floor ((value - spec.min) / spec.step + 0.5f);

        return (value <= spec.min || spec.max <= spec.min) ? spec.min : (value >= spec.max ? spec.max : value);
    }

    // As RangedAudioParameter::convertTo0to1, which snaps first
    inline float toProportion (const Spec& spec, float value)
    {
        value = snapToLegalValue (spec, value);

        switch (spec.kind)
        {
            case Kind::integer: return std::clamp ((value - spec.min) / (spec.max - spec.min), 0.0f, 1.0f);
            case Kind::choice:  return std::clamp (value / spec.max, 0.0f, 1.0f);
            case Kind::real:    break;
        }

        const auto proportion = std::clamp ((value - spec.min) / (spec.max - spec.min), 0.0f, 1.0f);
        return spec.skew == 1.0f ? proportion : std::pow (proportion, spec.skew);
    }

    // As NormalisableRange::convertFrom0to1, which doesn't snap
    inline float fromProportion (const Spec& spec, float proportion)
    {
        proportion = std::clamp (proportion, 0.0f, 1.0f);

        switch (spec.kind)
        {
            case Kind::integer: return std::clamp (proportion * (spec.max - spec.min) + spec.min, spec.min, spec.max);
            case Kind::choice:  return std::clamp (proportion * spec.max, 0.0f, spec.max);
            case Kind::real:    break;
        }

        if (spec.skew != 1.0f && proportion > 0.0f)
            proportion = std::exp (std::log (proportion) / spec.skew);

        return spec.min + (spec.max - spec.min) * proportion;
    }
}
