#include "WavetableVoice.h"
#include "ModulateSynth.h"

//==============================================================================
WavetableVoice::WavetableVoice(const juce::AudioBuffer<float> wavetablesToUse[3], ModulateSynth& synth)
    : wavetables(wavetablesToUse),
      owner(synth)
{
}

bool WavetableVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return withinVoiceLimit && dynamic_cast<WavetableSound*>(sound) != nullptr;
}

void WavetableVoice::startNote(int midiNoteNumber, float velocity,
                               juce::SynthesiserSound* sound,
                               int currentPitchWheelPosition)
{
    juce::ignoreUnused(sound, currentPitchWheelPosition);
    
    currentMidiNote = midiNoteNumber;
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    level = velocity;

    const bool   legato       = owner.getHeldNoteCount() > 0;
    const float  glideTime    = owner.modulated(ParameterSpecs::glideTime);
    const double glideSeconds = (legato && glideTime > 0.0f) ? glideTime : 0.0;

    const double startRatio = (glideSeconds > 0.0)
        ? juce::MidiMessage::getMidiNoteInHertz(juce::roundToInt(owner.getLastNotePitch())) / currentFrequency
        : 1.0;

    glideRatio.reset(getSampleRate(), glideSeconds);
    glideRatio.setCurrentAndTargetValue(startRatio);
    glideRatio.setTargetValue(1.0);

    for (int osc = 0; osc < 3; ++osc)
    {
        const auto size = wavetables[osc].getNumSamples();
        currentPhases[osc][0] = 0.0;
        for (int u = 1; u < ParameterSpecs::maxUnison; ++u)
            currentPhases[osc][u] = unisonRandom.nextDouble() * size;
    }
    
    noise.reset();

    // Initialize and trigger envelope
    owner.configureEnvelope(envelope, ParameterSpecs::env[0], getSampleRate());
    envelope.noteOn();

    owner.startModulationSources(getSampleRate());
}

void WavetableVoice::stopNote(float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity);

    if (allowTailOff)
    {
        // Trigger envelope release
        envelope.noteOff();
        
        owner.releaseModulationEnvelopes();
    }
    else
    {
        // Immediately stop the note
        clearCurrentNote();
        envelope.reset();
        
        owner.resetModulationEnvelopes();
        
        level = 0.0;
    }
}

void WavetableVoice::pitchWheelMoved(int newPitchWheelValue)
{
    juce::ignoreUnused(newPitchWheelValue);
}

void WavetableVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    juce::ignoreUnused(controllerNumber, newControllerValue);
}

void WavetableVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                     int startSample, int numSamples)
{
    if (!envelope.isActive())
    {
        clearCurrentNote();
        return;
    }

    const int wavetableSize = wavetables[0].getNumSamples();

    const double currentGlideRatio = glideRatio.getCurrentValue();
    glideRatio.skip(numSamples);

    const int   unisonCount  = juce::jlimit(1, ParameterSpecs::maxUnison,
                                            juce::roundToInt(owner.modulated(ParameterSpecs::unisonVoices)));
    const float detuneCents  = static_cast<float>(juce::roundToInt(owner.modulated(ParameterSpecs::unisonDetune)));
    const float spreadAmount = static_cast<float>(juce::roundToInt(owner.modulated(ParameterSpecs::unisonSpread))) / 100.0f; // 0..1
    const float unisonGain   = 1.0f / std::sqrt(static_cast<float>(unisonCount)); // level compensation

    double unisonRatio[ParameterSpecs::maxUnison];
    float  unisonPanL[ParameterSpecs::maxUnison];
    float  unisonPanR[ParameterSpecs::maxUnison];
    for (int u = 0; u < unisonCount; ++u)
    {
        const double pos = (unisonCount == 1) ? 0.0
                                              : -1.0 + 2.0 * u / (unisonCount - 1); // -1..+1
        unisonRatio[u] = std::pow(2.0, (pos * detuneCents) / 1200.0);

        const float panPos = static_cast<float>(pos) * spreadAmount; // -1..+1
        unisonPanL[u] = (panPos <= 0.0f) ? 1.0f : 1.0f - panPos;
        unisonPanR[u] = (panPos >= 0.0f) ? 1.0f : 1.0f + panPos;
    }

    bool         oscEnabled[3];
    float        oscGain[3];
    const float* oscData[3];
    double       oscPhaseIncrement[3][ParameterSpecs::maxUnison];

    const float  coarseSemis = owner.modulated(ParameterSpecs::coarsePitch);
    const double coarseRatio = std::pow(2.0, coarseSemis / 12.0);

    const bool  noiseEnabled = owner.isOn(ParameterSpecs::noiseEnable);
    const float noiseLevel   = owner.modulated(ParameterSpecs::noiseLevel);
    const auto  noiseType    = static_cast<NoiseGenerator::Type>(owner.choiceOf(ParameterSpecs::noiseType));

    for (int osc = 0; osc < 3; ++osc)
    {
        const auto& specs = ParameterSpecs::osc[static_cast<size_t>(osc)];

        oscEnabled[osc] = owner.isOn(specs.enable);
        oscGain[osc]    = owner.modulated(specs.gain);
        oscData[osc]    = wavetables[osc].getReadPointer(0); // cached pointer, hoisted out of the sample loop

        const int    pitchOffset   = juce::roundToInt(owner.modulated(specs.pitch));
        const double fineRatio     = std::pow(2.0, owner.modulated(specs.fine) / 1200.0);
        const double oscFrequency  = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote + pitchOffset) * currentGlideRatio * coarseRatio * fineRatio;
        const double baseIncrement = oscFrequency * wavetableSize / getSampleRate();

        for (int u = 0; u < unisonCount; ++u)
            oscPhaseIncrement[osc][u] = baseIncrement * unisonRatio[u];
    }

    const int numChannels = outputBuffer.getNumChannels();
    auto* const* channelPointers = outputBuffer.getArrayOfWritePointers();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = 0.0f, right = 0.0f;

        for (int osc = 0; osc < 3; ++osc)
        {
            if (! oscEnabled[osc])
                continue;

            const float* data = oscData[osc];

            for (int u = 0; u < unisonCount; ++u)
            {
                double& phase = currentPhases[osc][u];

                const float s = getInterpolatedSample(data, wavetableSize, phase) * oscGain[osc];
                left  += s * unisonPanL[u];
                right += s * unisonPanR[u];

                phase += oscPhaseIncrement[osc][u];
                while (phase >= wavetableSize)
                    phase -= wavetableSize;
            }
        }

        const float envelopeValue = envelope.getNextSample();
        const float voiceGain     = level * envelopeValue; 

        left  *= unisonGain * voiceGain;
        right *= unisonGain * voiceGain;

        if (noiseEnabled)
        {
            const float n = noise.getNextSample(noiseType) * noiseLevel * voiceGain;
            left  += n;
            right += n;
        }

        const int sampleIndex = startSample + sample;

        if (numChannels >= 2)
        {
            channelPointers[0][sampleIndex] += left;
            channelPointers[1][sampleIndex] += right;

            // Any channels beyond stereo get the mono downmix.
            for (int channel = 2; channel < numChannels; ++channel)
                channelPointers[channel][sampleIndex] += (left + right) * 0.5f;
        }
        else if (numChannels == 1)
        {
            channelPointers[0][sampleIndex] += (left + right) * 0.5f;
        }
    }
}

float WavetableVoice::getInterpolatedSample(const float* data, int size, double phase)
{
    const int   index0 = static_cast<int>(phase);
    const int   index1 = (index0 + 1) % size;
    const float frac   = static_cast<float>(phase - index0);

    return data[index0] + frac * (data[index1] - data[index0]);
}

//==============================================================================
WavetableSynthesiser::WavetableSynthesiser(ModulateSynth& synth)
    : owner(synth)
{
    setNoteStealingEnabled(true);
}

void WavetableSynthesiser::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
    owner.registerNoteStart(midiNoteNumber);
}

void WavetableSynthesiser::noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff)
{
    owner.registerNoteStop(midiNoteNumber);
    juce::Synthesiser::noteOff(midiChannel, midiNoteNumber, velocity, allowTailOff);
}

void WavetableSynthesiser::allNotesOff(int midiChannel, bool allowTailOff)
{
    juce::Synthesiser::allNotesOff(midiChannel, allowTailOff);
    owner.clearHeldNotes();
}
