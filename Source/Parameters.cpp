#include "Parameters.h"
#include "synth/WavetableGenerator.h"

namespace Parameters
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        constexpr int versionHint = 1;

        auto waveformNames = WavetableGenerator::getWaveformNames();

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"master_gain", versionHint}, "Master Gain",
            juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f), 0.0f));

        const bool oscDefaults[3] = { true, false, false };
        const int waveformDefaults[3] = { 0, 1, 2 };
        
        for (int i = 1; i <= 3; ++i)
        {
            juce::String oscPrefix = "osc" + juce::String(i);
            juce::String oscName = "Osc " + juce::String(i);
            
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{oscPrefix + "_enable", versionHint},
                oscName + " Enable", juce::StringArray { "Off", "On" }, oscDefaults[i - 1] ? 1 : 0));
            
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{oscPrefix + "_waveform", versionHint},
                oscName + " Waveform", waveformNames, waveformDefaults[i - 1]));

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{oscPrefix + "_waveform_b", versionHint},
                oscName + " Waveform B", waveformNames, waveformDefaults[i - 1]));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{oscPrefix + "_morph", versionHint},
                oscName + " Morph",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{oscPrefix + "_gain", versionHint},
                oscName + " Gain",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
            
            params.push_back(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID{oscPrefix + "_pitch", versionHint},
                oscName + " Pitch", -36, 36, 0));

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID{oscPrefix + "_fine", versionHint},
                oscName + " Fine", -99, 99, 0));
        }
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"noise_enable", versionHint}, "Noise Enable",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"noise_type", versionHint}, "Noise Type",
            juce::StringArray { "White", "Pink" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"noise_level", versionHint}, "Noise Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        auto syncRates = juce::StringArray {
            "1/16", "1/16T", "1/16D", "1/8", "1/8T", "1/8D",
            "1/4", "1/4T", "1/4D", "1/2", "1/2T", "1/2D",
            "1 Bar", "2 Bars", "4 Bars"
        };
        const float lfoRateDefaults[4] = { 1.0f, 2.0f, 4.0f, 8.0f };
        
        for (int i = 1; i <= 4; ++i)
        {
            juce::String lfoPrefix = "lfo" + juce::String(i);
            juce::String lfoName = "LFO " + juce::String(i);
            
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{lfoPrefix + "_mode", versionHint},
                lfoName + " Mode",
                juce::StringArray { "Trigger", "Sync" }, 0));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{lfoPrefix + "_rate", versionHint},
                lfoName + " Rate",
                juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f),
                lfoRateDefaults[i - 1]));
            
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{lfoPrefix + "_sync", versionHint},
                lfoName + " Sync", juce::StringArray { "Off", "On" }, 0));
            
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{lfoPrefix + "_sync_rate", versionHint},
                lfoName + " Sync Rate", syncRates, 6));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{lfoPrefix + "_tension", versionHint},
                lfoName + " Tension",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        }
        
        // Filter parameters
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"filter_enable", versionHint}, "Filter Enable",
            juce::StringArray { "Off", "On" }, 1));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"filter_mode", versionHint}, "Filter Mode",
            juce::StringArray { "Lowpass", "Highpass", "Bandpass", "Notch" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"filter_slope", versionHint}, "Filter Slope",
            juce::StringArray { "12 dB", "24 dB" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"filter_cutoff", versionHint}, "Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"filter_resonance", versionHint}, "Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"drive", versionHint}, "Drive",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"drive_hq", versionHint}, "Drive HQ",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"glide_time", versionHint}, "Glide Time",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.4f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"coarse_pitch", versionHint}, "Coarse Pitch",
            juce::NormalisableRange<float>(-36.0f, 36.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"num_voices", versionHint}, "Voices", 1, maxVoices, maxVoices));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"unison_voices", versionHint}, "Unison", 1, maxUnison, 1));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"unison_detune", versionHint}, "Detune", 0, 100, 15));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"unison_spread", versionHint}, "Spread", 0, 100, 50));

        for (int i = 1; i <= 4; ++i)
        {
            juce::String envPrefix = "env" + juce::String(i);
            juce::String envName = "Env " + juce::String(i);
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_attack", versionHint}, envName + " Attack",
                juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_decay", versionHint}, envName + " Decay",
                juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_sustain", versionHint}, envName + " Sustain",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_release", versionHint}, envName + " Release",
                juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.3f));
        }
        
        return { params.begin(), params.end() };
    }
}
