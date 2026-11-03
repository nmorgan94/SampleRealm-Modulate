#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using Catch::Matchers::WithinAbs;

//==============================================================================
TEST_CASE_METHOD (TestHelpers::ProcessorFixture, "Parameters match the snapshot hosts rely on", "[parameters]")
{
    struct Expected
    {
        const char* id;
        int versionHint;
        const char* name;
        const char* label;
        float start, end, interval, skew, defaultValue;
        std::array<const char*, 3> textAt0HalfAnd1;
    };

    const std::vector<Expected> snapshot {
        { "master_gain", 1, "Master Gain", "", -24.0f, 6.0f, 0.1f, 1.0f, 0.0f, { "-24.0", "-9.0", "6.0" } },
        { "osc1_enable", 1, "Osc 1 Enable", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, { "Off", "Off", "On" } },
        { "osc1_waveform", 1, "Osc 1 Waveform", "", 0.0f, 20.0f, 1.0f, 1.0f, 0.0f, { "Sine", "Feedback Fm", "Trapezoid" } },
        { "osc1_waveform_b", 1, "Osc 1 Waveform B", "", 0.0f, 20.0f, 1.0f, 1.0f, 0.0f, { "Sine", "Feedback Fm", "Trapezoid" } },
        { "osc1_morph", 1, "Osc 1 Morph", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.0f, { "0.00", "0.50", "1.00" } },
        { "osc1_gain", 1, "Osc 1 Gain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "osc1_pitch", 1, "Osc 1 Pitch", "", -36.0f, 36.0f, 1.0f, 1.0f, 0.0f, { "-36", "0", "36" } },
        { "osc1_fine", 1, "Osc 1 Fine", "", -99.0f, 99.0f, 1.0f, 1.0f, 0.0f, { "-99", "0", "99" } },
        { "osc2_enable", 1, "Osc 2 Enable", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "osc2_waveform", 1, "Osc 2 Waveform", "", 0.0f, 20.0f, 1.0f, 1.0f, 1.0f, { "Sine", "Feedback Fm", "Trapezoid" } },
        { "osc2_waveform_b", 1, "Osc 2 Waveform B", "", 0.0f, 20.0f, 1.0f, 1.0f, 1.0f, { "Sine", "Feedback Fm", "Trapezoid" } },
        { "osc2_morph", 1, "Osc 2 Morph", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.0f, { "0.00", "0.50", "1.00" } },
        { "osc2_gain", 1, "Osc 2 Gain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "osc2_pitch", 1, "Osc 2 Pitch", "", -36.0f, 36.0f, 1.0f, 1.0f, 0.0f, { "-36", "0", "36" } },
        { "osc2_fine", 1, "Osc 2 Fine", "", -99.0f, 99.0f, 1.0f, 1.0f, 0.0f, { "-99", "0", "99" } },
        { "osc3_enable", 1, "Osc 3 Enable", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "osc3_waveform", 1, "Osc 3 Waveform", "", 0.0f, 20.0f, 1.0f, 1.0f, 2.0f, { "Sine", "Feedback Fm", "Trapezoid" } },
        { "osc3_waveform_b", 1, "Osc 3 Waveform B", "", 0.0f, 20.0f, 1.0f, 1.0f, 2.0f, { "Sine", "Feedback Fm", "Trapezoid" } },
        { "osc3_morph", 1, "Osc 3 Morph", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.0f, { "0.00", "0.50", "1.00" } },
        { "osc3_gain", 1, "Osc 3 Gain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "osc3_pitch", 1, "Osc 3 Pitch", "", -36.0f, 36.0f, 1.0f, 1.0f, 0.0f, { "-36", "0", "36" } },
        { "osc3_fine", 1, "Osc 3 Fine", "", -99.0f, 99.0f, 1.0f, 1.0f, 0.0f, { "-99", "0", "99" } },
        { "noise_enable", 1, "Noise Enable", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "noise_type", 1, "Noise Type", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "White", "White", "Pink" } },
        { "noise_level", 1, "Noise Level", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.5f, { "0.00", "0.50", "1.00" } },
        { "lfo1_mode", 1, "LFO 1 Mode", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Trigger", "Trigger", "Sync" } },
        { "lfo1_rate", 1, "LFO 1 Rate", "", 0.01f, 20.0f, 0.01f, 0.3f, 1.0f, { "0.01", "1.99", "20.00" } },
        { "lfo1_sync", 1, "LFO 1 Sync", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "lfo1_sync_rate", 1, "LFO 1 Sync Rate", "", 0.0f, 14.0f, 1.0f, 1.0f, 6.0f, { "1/16", "1/4T", "4 Bars" } },
        { "lfo1_tension", 1, "LFO 1 Tension", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.5f, { "0.00", "0.50", "1.00" } },
        { "lfo2_mode", 1, "LFO 2 Mode", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Trigger", "Trigger", "Sync" } },
        { "lfo2_rate", 1, "LFO 2 Rate", "", 0.01f, 20.0f, 0.01f, 0.3f, 2.0f, { "0.01", "1.99", "20.00" } },
        { "lfo2_sync", 1, "LFO 2 Sync", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "lfo2_sync_rate", 1, "LFO 2 Sync Rate", "", 0.0f, 14.0f, 1.0f, 1.0f, 6.0f, { "1/16", "1/4T", "4 Bars" } },
        { "lfo2_tension", 1, "LFO 2 Tension", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.5f, { "0.00", "0.50", "1.00" } },
        { "lfo3_mode", 1, "LFO 3 Mode", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Trigger", "Trigger", "Sync" } },
        { "lfo3_rate", 1, "LFO 3 Rate", "", 0.01f, 20.0f, 0.01f, 0.3f, 4.0f, { "0.01", "1.99", "20.00" } },
        { "lfo3_sync", 1, "LFO 3 Sync", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "lfo3_sync_rate", 1, "LFO 3 Sync Rate", "", 0.0f, 14.0f, 1.0f, 1.0f, 6.0f, { "1/16", "1/4T", "4 Bars" } },
        { "lfo3_tension", 1, "LFO 3 Tension", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.5f, { "0.00", "0.50", "1.00" } },
        { "lfo4_mode", 1, "LFO 4 Mode", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Trigger", "Trigger", "Sync" } },
        { "lfo4_rate", 1, "LFO 4 Rate", "", 0.01f, 20.0f, 0.01f, 0.3f, 8.0f, { "0.01", "1.99", "20.00" } },
        { "lfo4_sync", 1, "LFO 4 Sync", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "lfo4_sync_rate", 1, "LFO 4 Sync Rate", "", 0.0f, 14.0f, 1.0f, 1.0f, 6.0f, { "1/16", "1/4T", "4 Bars" } },
        { "lfo4_tension", 1, "LFO 4 Tension", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.5f, { "0.00", "0.50", "1.00" } },
        { "filter_enable", 1, "Filter Enable", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, { "Off", "Off", "On" } },
        { "filter_mode", 1, "Filter Mode", "", 0.0f, 3.0f, 1.0f, 1.0f, 0.0f, { "Lowpass", "Bandpass", "Notch" } },
        { "filter_slope", 1, "Filter Slope", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "12 dB", "12 dB", "24 dB" } },
        { "filter_cutoff", 1, "Filter Cutoff", "", 20.0f, 20000.0f, 1.0f, 0.3f, 1000.0f, { "20", "2002", "20000" } },
        { "filter_resonance", 1, "Filter Resonance", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.5f, { "0.00", "0.50", "1.00" } },
        { "drive", 1, "Drive", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.0f, { "0.00", "0.50", "1.00" } },
        { "drive_hq", 1, "Drive HQ", "", 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, { "Off", "Off", "On" } },
        { "glide_time", 1, "Glide Time", "", 0.0f, 2.0f, 0.001f, 0.4f, 0.0f, { "0.000", "0.354", "2.000" } },
        { "coarse_pitch", 1, "Coarse Pitch", "", -36.0f, 36.0f, 0.01f, 1.0f, 0.0f, { "-36.00", "-0.00", "36.00" } },
        { "num_voices", 1, "Voices", "", 1.0f, 8.0f, 1.0f, 1.0f, 8.0f, { "1", "4", "8" } },
        { "unison_voices", 1, "Unison", "", 1.0f, 7.0f, 1.0f, 1.0f, 1.0f, { "1", "4", "7" } },
        { "unison_detune", 1, "Detune", "", 0.0f, 100.0f, 1.0f, 1.0f, 15.0f, { "0", "50", "100" } },
        { "unison_spread", 1, "Spread", "", 0.0f, 100.0f, 1.0f, 1.0f, 50.0f, { "0", "50", "100" } },
        { "env1_attack", 1, "Env 1 Attack", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.01f, { "0.001", "0.497", "5.000" } },
        { "env1_decay", 1, "Env 1 Decay", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.1f, { "0.001", "0.497", "5.000" } },
        { "env1_sustain", 1, "Env 1 Sustain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "env1_release", 1, "Env 1 Release", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.3f, { "0.001", "0.497", "5.000" } },
        { "env2_attack", 1, "Env 2 Attack", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.01f, { "0.001", "0.497", "5.000" } },
        { "env2_decay", 1, "Env 2 Decay", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.1f, { "0.001", "0.497", "5.000" } },
        { "env2_sustain", 1, "Env 2 Sustain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "env2_release", 1, "Env 2 Release", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.3f, { "0.001", "0.497", "5.000" } },
        { "env3_attack", 1, "Env 3 Attack", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.01f, { "0.001", "0.497", "5.000" } },
        { "env3_decay", 1, "Env 3 Decay", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.1f, { "0.001", "0.497", "5.000" } },
        { "env3_sustain", 1, "Env 3 Sustain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "env3_release", 1, "Env 3 Release", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.3f, { "0.001", "0.497", "5.000" } },
        { "env4_attack", 1, "Env 4 Attack", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.01f, { "0.001", "0.497", "5.000" } },
        { "env4_decay", 1, "Env 4 Decay", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.1f, { "0.001", "0.497", "5.000" } },
        { "env4_sustain", 1, "Env 4 Sustain", "", 0.0f, 1.0f, 0.01f, 1.0f, 0.7f, { "0.00", "0.50", "1.00" } },
        { "env4_release", 1, "Env 4 Release", "", 0.001f, 5.0f, 0.001f, 0.3f, 0.3f, { "0.001", "0.497", "5.000" } },
    };

    const auto& parameters = processor.getParameters();
    REQUIRE (static_cast<size_t> (parameters.size()) == snapshot.size());

    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        const auto& expected = snapshot[i];
        CAPTURE (i, expected.id);

        auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (parameters[static_cast<int> (i)]);
        REQUIRE (parameter != nullptr);

        CHECK (parameter->getParameterID() == expected.id);
        CHECK (parameter->getVersionHint() == expected.versionHint);
        CHECK (parameter->getName (100) == expected.name);
        CHECK (parameter->getLabel() == expected.label);

        const auto& range = parameter->getNormalisableRange();
        CHECK_THAT (range.start, WithinAbs (expected.start, 1.0e-6));
        CHECK_THAT (range.end, WithinAbs (expected.end, 1.0e-6));
        CHECK_THAT (range.interval, WithinAbs (expected.interval, 1.0e-6));
        CHECK_THAT (range.skew, WithinAbs (expected.skew, 1.0e-6));
        // Coarse Pitch's default snaps to -0.000001
        CHECK_THAT (parameter->convertFrom0to1 (parameter->getDefaultValue()), WithinAbs (expected.defaultValue, 1.0e-5));

        CHECK (parameter->getText (0.0f, 100) == expected.textAt0HalfAnd1[0]);
        CHECK (parameter->getText (0.5f, 100) == expected.textAt0HalfAnd1[1]);
        CHECK (parameter->getText (1.0f, 100) == expected.textAt0HalfAnd1[2]);
    }
}

TEST_CASE_METHOD (TestHelpers::ProcessorFixture, "The specs' ranges convert as the parameters' do", "[parameters]")
{
    for (const auto* spec : ParameterSpecs::all)
    {
        CAPTURE (spec->id);

        auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (processor.getAPVTS().getParameter (spec->id));
        REQUIRE (parameter != nullptr);
        const auto& range = parameter->getNormalisableRange();

        for (int step = 0; step <= 200; ++step)
        {
            const auto proportion = static_cast<float> (step) / 200.0f;
            const auto value = range.convertFrom0to1 (proportion);
            CAPTURE (proportion);

            CHECK (juce::exactlyEqual (ParameterSpecs::fromProportion (*spec, proportion), value));
            CHECK (juce::exactlyEqual (ParameterSpecs::snapToLegalValue (*spec, value), range.snapToLegalValue (value)));
            CHECK (juce::exactlyEqual (ParameterSpecs::toProportion (*spec, value), parameter->convertTo0to1 (value)));
        }
    }
}
