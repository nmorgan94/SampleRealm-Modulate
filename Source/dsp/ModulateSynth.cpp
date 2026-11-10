#include "ModulateSynth.h"

#include <algorithm>
#include <utility>

using namespace ParameterSpecs;

//==============================================================================
ModulateSynth::ModulateSynth() = default;

ModulateSynth::Values ModulateSynth::defaultValues()
{
    Values values;

    for (size_t i = 0; i < values.size(); ++i)
        values[i] = all[i]->defaultValue;

    return values;
}

float ModulateSynth::modulatedValue(const Spec& spec, float value, const Modulation::Assignment& assignment, float sourceValue)
{
    if (! assignment.isAssigned())
        return fromProportion(spec, toProportion(spec, value));

    return fromProportion(spec, Modulation::modulatedProportion(assignment, sourceValue));
}

//==============================================================================
void ModulateSynth::prepare(double sampleRate, int maximumBlockSize, int numChannels)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (size_t i = 0; i < std::size(lfos); ++i)
    {
        lfos[i].prepare(sampleRate);
        lfos[i].setRate(valueOf(lfo[i].rate));
    }

    updateLfoShapes(true);

    for (size_t i = 0; i < std::size(envelopes); ++i)
        configureEnvelope(envelopes[i], env[i], sampleRate);

    WavetableBank::get(WaveformType::Sine);   // builds the bank here, not on the audio thread

    filter.prepare(sampleRate);
    updateFilter();

    drive.prepare(maximumBlockSize, juce::jmax(1, numChannels));

    synth.clearVoices();
    synth.clearSounds();
    voices.clear();

    synth.addSound(new WavetableSound());

    for (int i = 0; i < maxVoices; ++i)
    {
        auto* voice = new WavetableVoice(*this);
        synth.addVoice(voice);
        voices.push_back(voice);
    }

    currentVoiceCount = -1;
    updateVoiceCount();

    legato.clear();
}

void ModulateSynth::setSettings(const Settings& newSettings)
{
    settings = newSettings;
    updateLfoShapes(false);
}

void ModulateSynth::process(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    const auto numSamples = buffer.getNumSamples();

    for (auto& lfoToSync : lfos)
        lfoToSync.setTempoInfo(settings.tempo.bpm, settings.tempo.numerator, settings.tempo.denominator);

    // These read the modulation sources before they advance
    updateLfos();
    updateVoiceCount();

    advanceModulationSources(numSamples);

    updateFilter();
    updateOscillatorTables();

    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, numSamples);

    if (isOn(filterEnable))
    {
        drive.setDrive(modulated(ParameterSpecs::drive));
        drive.process(buffer, isOn(driveHq));
        filter.process(buffer);
    }

    buffer.applyGain(juce::Decibels::decibelsToGain(valueOf(masterGain)));

    publishLive();
}

//==============================================================================
float ModulateSynth::modulated(const Spec& spec) const
{
    const auto index = indexOf(spec);
    const auto& assignment = settings.modulation[index];
    return modulatedValue(spec, settings.values[index], assignment, sourceValue(assignment));
}

float ModulateSynth::sourceValue(const Modulation::Assignment& assignment) const
{
    if (! assignment.isAssigned())
        return 0.0f;

    return assignment.isLFO() ? lfos[assignment.sourceIndex].getCurrentValue()
                              : envelopes[assignment.sourceIndex].getCurrentLevel();
}

void ModulateSynth::configureEnvelope(ADSREnvelope& envelope, const ParameterSpecs::Envelope& specs, double sampleRate) const
{
    envelope.setSampleRate(sampleRate);
    envelope.setParameters(valueOf(specs.attack), valueOf(specs.decay), valueOf(specs.sustain), valueOf(specs.release));
}

//==============================================================================
void ModulateSynth::startModulationSources(double sampleRate)
{
    for (size_t i = 0; i < std::size(envelopes); ++i)
    {
        configureEnvelope(envelopes[i], env[i], sampleRate);
        envelopes[i].noteOn();
    }

    // Only those in trigger mode restart
    for (auto& lfoToTrigger : lfos)
        lfoToTrigger.trigger();
}

void ModulateSynth::releaseModulationEnvelopes()
{
    for (auto& envelope : envelopes)
        envelope.noteOff();
}

void ModulateSynth::resetModulationEnvelopes()
{
    for (auto& envelope : envelopes)
        envelope.reset();
}

//==============================================================================
void ModulateSynth::updateLfoShapes(bool force)
{
    for (size_t i = 0; i < numLfos; ++i)
    {
        const auto& curve = settings.curves[i];
        const auto tension = valueOf(lfo[i].tension);

        if (! force && curve == lfoCurvesShown[i] && juce::exactlyEqual(tension, lfoTensionsShown[i]))
            continue;

        lfoCurvesShown[i] = curve;
        lfoTensionsShown[i] = tension;
        lfos[i].setShape(curve.count == 0 ? std::span<const LfoCurve::Point>(LfoCurve::defaultPoints) : curve.view(), tension);
    }
}

void ModulateSynth::updateLfos()
{
    for (size_t i = 0; i < numLfos; ++i)
    {
        const auto& specs = lfo[i];
        lfos[i].setRate(modulated(specs.rate));
        lfos[i].setTriggerMode(choiceOf(specs.mode) == 0);
        lfos[i].setTempoSync(isOn(specs.sync));
        lfos[i].setTempoDivision(choiceOf(specs.syncRate));
    }
}

void ModulateSynth::updateVoiceCount()
{
    const int desiredVoices = juce::roundToInt(modulated(numVoices));

    if (desiredVoices == currentVoiceCount)
        return;

    currentVoiceCount = desiredVoices;

    for (size_t i = 0; i < voices.size(); ++i)
    {
        const bool isWithinLimit = std::cmp_less(i, desiredVoices);

        if (! isWithinLimit && voices[i]->isVoiceActive())
            voices[i]->stopNote(0.0f, true);

        voices[i]->setWithinVoiceLimit(isWithinLimit);
    }
}

void ModulateSynth::advanceModulationSources(int numSamples)
{
    const bool hasActiveVoices = std::ranges::any_of(voices, [](const WavetableVoice* voice) { return voice->isVoiceActive(); });

    for (int i = 0; i < numLfos; ++i)
    {
        const bool isAssigned = Modulation::isSourceAssigned(settings.modulation, Modulation::Source::LFO, i);

        if (lfos[i].isTriggerMode())
        {
            // Back to the start when no notes are playing or nothing listens
            if (isAssigned && hasActiveVoices)
                lfos[i].advance(numSamples);
            else
                lfos[i].trigger();
        }
        else if (isAssigned)
        {
            lfos[i].advance(numSamples);
        }
    }

    // Envelope 1 always runs, for the editor's playhead
    for (int i = 0; i < numEnvelopes; ++i)
    {
        const bool shouldAdvance = (i == 0) || Modulation::isSourceAssigned(settings.modulation, Modulation::Source::Envelope, i);
        if (shouldAdvance && envelopes[i].isActive())
        {
            for (int sample = 0; sample < numSamples; ++sample)
                envelopes[i].getNextSample();
        }
    }
}

void ModulateSynth::updateFilter()
{
    filter.setParameters(static_cast<FilterMode>(choiceOf(filterMode)),
                         static_cast<FilterSlope>(choiceOf(filterSlope)),
                         modulated(filterCutoff),
                         modulated(filterResonance));
}

void ModulateSynth::updateOscillatorTables()
{
    for (size_t i = 0; i < std::size(oscillatorTables); ++i)
    {
        const auto& specs = osc[i];
        const auto a = static_cast<WaveformType>(choiceOf(specs.waveform));
        const auto b = static_cast<WaveformType>(choiceOf(specs.waveformB));
        const float morph = modulated(specs.morph);
        auto& tables = oscillatorTables[i];

        if (a == tables.a && b == tables.b && juce::exactlyEqual(morph, tables.morph))
            continue;

        tables.a = a;
        tables.b = b;
        tables.morph = morph;
        tables.blendedLevels = 0;   // every blended level is stale
    }
}

// Blends only the levels the voices ask for
const float* ModulateSynth::tableFor(int oscIndex, int level)
{
    auto& tables = oscillatorTables[oscIndex];
    const auto& a = WavetableBank::get(tables.a);
    const auto& b = WavetableBank::get(tables.b);

    // All A or all B: read the bank, no blend
    if (tables.a == tables.b || tables.morph <= 0.0f)
        return a.getReadPointer(level);

    if (tables.morph >= 1.0f)
        return b.getReadPointer(level);

    if ((tables.blendedLevels & (1u << level)) == 0)
    {
        auto* out = tables.blended.getWritePointer(level);
        juce::FloatVectorOperations::copyWithMultiply(out, a.getReadPointer(level), 1.0f - tables.morph, WavetableBank::size);
        juce::FloatVectorOperations::addWithMultiply(out, b.getReadPointer(level), tables.morph, WavetableBank::size);
        tables.blendedLevels |= 1u << level;
    }

    return tables.blended.getReadPointer(level);
}

void ModulateSynth::publishLive()
{
    for (size_t i = 0; i < numLfos; ++i)
    {
        live.lfoPhase[i] = lfos[i].getCurrentPhase();
        live.lfoValue[i] = lfos[i].getCurrentValue();
    }

    for (size_t i = 0; i < numEnvelopes; ++i)
    {
        live.envelopeLevel[i] = envelopes[i].getCurrentLevel();
        live.envelopeProgress[i] = envelopes[i].getStageProgress();
        live.envelopeStage[i] = envelopes[i].getState();
    }
}
