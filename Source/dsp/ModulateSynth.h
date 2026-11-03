#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>
#include "../shared/ParameterSpecs.h"
#include "ADSREnvelope.h"
#include "Drive.h"
#include "Filter.h"
#include "LFO.h"
#include "LfoCurve.h"
#include "Modulation.h"
#include "WavetableGenerator.h"
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

    struct Tempo
    {
        double bpm = 120.0;
        int numerator = 4, denominator = 4;
    };

    // Values and assignments are in ParameterSpecs::all's order
    struct Settings
    {
        Values values = defaultValues();
        Assignments modulation {};
        Curves curves {};
        Tempo tempo;
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
    void process(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);

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

    void configureEnvelope(ADSREnvelope& envelope, const ParameterSpecs::Envelope& specs, double sampleRate) const;
    void startModulationSources(double sampleRate);
    void releaseModulationEnvelopes();
    void resetModulationEnvelopes();

    int getHeldNoteCount() const { return static_cast<int>(heldNotes.size()); }
    double getLastNotePitch() const { return heldNotes.empty() ? 60.0 : static_cast<double>(heldNotes.back()); }
    void registerNoteStart(int midiNote) { heldNotes.push_back(midiNote); }
    void registerNoteStop(int midiNote);
    void clearHeldNotes() { heldNotes.clear(); }

    void updateLfoShapes(bool force);
    void updateLfos();
    void updateVoiceCount();
    void advanceModulationSources(int numSamples);
    void updateFilter();
    void updateOscillatorTables(bool forceRegen);
    void blendWavetable(size_t oscIndex, float morph);
    void publishLive();

    Settings settings;
    Curves lfoCurvesShown {};
    std::array<float, numLfos> lfoTensionsShown {};

    WavetableSynthesiser synth { *this };
    std::vector<WavetableVoice*> voices;
    juce::AudioBuffer<float> wavetables[numOscillators];   // blended A/B output read by the audio voices
    juce::AudioBuffer<float> sourceA[numOscillators];      // pure waveform A (osc{i}_waveform)
    juce::AudioBuffer<float> sourceB[numOscillators];      // pure waveform B (osc{i}_waveform_b)
    LFO lfos[numLfos];
    ADSREnvelope envelopes[numEnvelopes];
    Filter filter;
    Drive drive;
    Live live;

    WaveformType currentWaveformTypes[numOscillators]  = { WaveformType::Sine, WaveformType::Sine, WaveformType::Sine };
    WaveformType currentWaveformTypesB[numOscillators] = { WaveformType::Sine, WaveformType::Sine, WaveformType::Sine };
    float lastMorph[numOscillators] = { -1.0f, -1.0f, -1.0f }; // -1 forces the first blend

    int currentVoiceCount = -1;
    std::vector<int> heldNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulateSynth)
};
