#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "ADSREnvelope.h"
#include "Glide.h"
#include "NoiseGenerator.h"
#include "../shared/ParameterSpecs.h"

class ModulateSynth;

//==============================================================================
class WavetableVoice : public juce::SynthesiserVoice
{
public:
    explicit WavetableVoice(ModulateSynth& synth);
    
    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void setWithinVoiceLimit(bool isWithinLimit) { withinVoiceLimit = isWithinLimit; }

    // Next note slides on without restarting
    void setSliding(bool shouldSlide) { sliding = shouldSlide; }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    
    void stopNote(float velocity, bool allowTailOff) override;
    
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

private:
    ModulateSynth& owner;
    
    double currentPhases[3][ParameterSpecs::maxUnison] = {};
    juce::Random unisonRandom;
    float level = 0.0f;

    GlideProcessor glide;
    bool sliding = false;

    ADSREnvelope envelope;
    NoiseGenerator noise;

    bool withinVoiceLimit = true;

    static float getInterpolatedSample(const float* data, double phase);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableVoice)
};

//==============================================================================
class WavetableSound : public juce::SynthesiserSound
{
public:
    WavetableSound() {}

    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSound)
};

//==============================================================================
// Mono legato with one voice
class WavetableSynthesiser : public juce::Synthesiser
{
public:
    explicit WavetableSynthesiser(ModulateSynth& synth);

    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;
    void allNotesOff(int midiChannel, bool allowTailOff) override;

private:
    ModulateSynth& owner;

    WavetableVoice* monoVoice() const;
    void slide(WavetableVoice& voice, int midiChannel, int midiNoteNumber, float velocity);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSynthesiser)
};
