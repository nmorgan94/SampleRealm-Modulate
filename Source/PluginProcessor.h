#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/ModulateSynth.h"
#include "dsp/PeakMeter.h"
#include "ModulationManager.h"
#include "Parameters.h"
#include "LFOCurveState.h"
#include "presets/PresetManager.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor,
                                         private juce::ValueTree::Listener
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    PresetManager& getPresetManager() { return presetManager; }

    static PresetManager::Config createPresetConfig();

    // Moves each time the state is replaced: a preset, Init or a session
    juce::uint32 getStateChanges() const noexcept { return stateChanges.load(); }

    //==============================================================================
    // The value the voices last heard, modulation included. For the editor
    float getModulatedParam(const juce::String& paramID) const;

    const ModulateSynth::Live& getLive() const { return synth.getLive(); }

    ModulationManager& getModulationManager() { return modulationManager; }
    
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    PeakMeter& getOutputMeter() { return outputMeter; }
    
    // LFO curve persistence
    std::vector<LfoCurve::Point> getLFOCurvePoints(size_t lfoIndex) const
    {
        jassert(lfoIndex < 4);
        return lfoCurveState.getCurvePoints(lfoIndex);
    }
    
    void setLFOCurvePoints(size_t lfoIndex, const std::vector<LfoCurve::Point>& points)
    {
        jassert(lfoIndex < 4);
        lfoCurveState.setCurvePoints(lfoIndex, points);
    }

    ModulateSynth::Curves getLFOCurves() const { return lfoCurveState.getCurves(); }

private:
    static inline const juce::Identifier modMatrixStateID { "ModMatrix" };
    static inline const juce::Identifier lfoCurvesStateID { "LFOCurves" };
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters",
                                               Parameters::createLayout() };
    juce::UndoManager undoManager;
    ModulationManager modulationManager;
    LFOCurveState lfoCurveState;
    PresetManager presetManager { apvts, createPresetConfig() };
    ModulateSynth synth;
    PeakMeter outputMeter { 2 };

    // The host's, kept from the last block that had one
    ModulateSynth::Tempo tempo;

    std::atomic<juce::uint32> stateChanges { 0 };
    
    static juce::ValueTree getOrCreateStateChild(juce::ValueTree parent, const juce::Identifier& childID);

    // Points the modulation/LFO-curve managers at their state children. Shared by the
    // constructor and every state-restore path.
    void reinitialiseStateManagers();

    void valueTreeRedirected (juce::ValueTree&) override;

    ModulateSynth::Settings currentSettings() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
