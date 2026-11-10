#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>
#include "../shared/ParameterSpecs.h"
#include "../shared/Transport.h"
#include "ADSREnvelope.h"
#include "Drive.h"
#include "Filter.h"
#include "Glide.h"
#include "LFO.h"
#include "LfoCurve.h"
#include "Modulation.h"
#include "WavetableBank.h"
#include "WavetableVoice.h"

//==============================================================================
// Eight wavetable voices, the four LFOs and four envelopes they share as modulation
// sources, then drive, filter and gain
class ModulateSynth
{
public:
    static constexpr int numOscillators = 3;
    static constexpr int numLfos = Modulation::numSources;
    static constexpr int numEnvelopes = Modulation::numSources;

    using Values = std::array<float, ParameterSpecs::all.size()>;
    using Assignments = std::array<Modulation::Assignment, ParameterSpecs::all.size()>;
    using Curves = std::array<LfoCurve::Points, numLfos>;

    // Values and assignments are in ParameterSpecs::all's order
    struct Settings
    {
        Values values = defaultValues();
        Assignments modulation {};
        Curves curves {};
    };

    // Written at the end of each block, for the editors
    struct Live
    {
        std::array<std::atomic<float>, numLfos> lfoPhase {}, lfoValue {};
        std::array<std::atomic<float>, numEnvelopes> envelopeLevel {}, envelopeProgress {};
        std::array<std::atomic<ADSREnvelope::State>, numEnvelopes> envelopeStage {};

        float sourceValue(const Modulation::Assignment& assignment) const
        {
            if (! assignment.isAssigned())
                return 0.0f;

            const auto index = static_cast<size_t>(assignment.sourceIndex);
            return assignment.isLFO() ? lfoValue[index].load() : envelopeLevel[index].load();
        }
    };

    // Out of line, as Settings' defaults need the whole class
    ModulateSynth();

    void prepare(double sampleRate, int maximumBlockSize, int numChannels);
    void setSettings(const Settings& newSettings);
    void process(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport);

    const Live& getLive() const noexcept { return live; }

    static Values defaultValues();

    // A parameter's value with its modulation applied, as the voices hear it
    static float modulatedValue(const ParameterSpecs::Spec& spec, float value,
                                const Modulation::Assignment& assignment, float sourceValue);

private:
    friend class WavetableVoice;
    friend class WavetableSynthesiser;

    float valueOf(const ParameterSpecs::Spec& spec) const { return settings.values[ParameterSpecs::indexOf(spec)]; }
    bool isOn(const ParameterSpecs::Spec& spec) const { return choiceOf(spec) != 0; }
    int choiceOf(const ParameterSpecs::Spec& spec) const { return juce::roundToInt(valueOf(spec)); }
    float modulated(const ParameterSpecs::Spec& spec) const;
    float sourceValue(const Modulation::Assignment& assignment) const;

    void startModulationSources();
    void releaseModulationEnvelopes();
    void resetModulationEnvelopes();

    GlideMode glideMode() const { return static_cast<GlideMode>(choiceOf(ParameterSpecs::glideMode)); }
    bool isMono() const { return currentVoiceCount == 1; }

    void updateLfoShapes(bool force);
    void updateEnvelopeParameters();
    void updateLfos();
    void updateVoiceCount();
    void advanceModulationSources(int numSamples, const Transport& transport);
    void updateFilter();
    void updateOscillatorTables();
    const float* tableFor(int oscIndex, int level);   // for voices, once per block
    void publishLive();

    Settings settings;
    Curves lfoCurvesShown {};
    std::array<float, numLfos> lfoTensionsShown {};

    WavetableSynthesiser synth { *this };
    std::vector<WavetableVoice*> voices;

    // One oscillator's A and B, and their mix, blended a level at a time as voices ask
    struct OscillatorTables
    {
        WaveformType a = WaveformType::Sine, b = WaveformType::Sine;
        float morph = 0.0f;
        juce::AudioBuffer<float> blended { WavetableBank::numLevels, WavetableBank::size };
        juce::uint32 blendedLevels = 0;   // bit L: level L of blended matches a, b and morph
    };
    OscillatorTables oscillatorTables[numOscillators];

    LFO lfos[numLfos];
    ADSREnvelope envelopes[numEnvelopes];
    ADSREnvelope::Parameters envelopeParameters[numEnvelopes];
    Filter filter;
    Drive drive;
    Live live;

    int currentVoiceCount = -1;
    Legato legato;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulateSynth)
};
