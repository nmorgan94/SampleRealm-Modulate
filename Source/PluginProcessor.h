#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/ModulateSynth.h"
#include "dsp/PeakMeter.h"
#include "ModulationManager.h"
#include "Parameters.h"
#include "LFOCurveState.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
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

    // Shared save/restore primitives, reused by both the DAW state path above and
    // the preset system. getStateTree() is the full tree (params + ModMatrix + LFOCurves);
    // applyStateTree() swaps it in and re-points the modulation/LFO-curve managers.
    juce::ValueTree getStateTree();
    bool applyStateTree(const juce::ValueTree& newState);  // false if newState wasn't a valid state tree

    // Resets all parameters to their defaults and clears modulation routing / LFO curves.
    // Backs the "Init" preset.
    void resetToDefaultState();
    juce::String getCurrentPresetName() const;
    void setCurrentPresetName(const juce::String& name);

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
    
    juce::AudioProcessorValueTreeState apvts;

private:
    static inline const juce::Identifier modMatrixStateID { "ModMatrix" };
    static inline const juce::Identifier lfoCurvesStateID { "LFOCurves" };
    static inline const juce::Identifier presetNameID    { "currentPreset" };
    //==============================================================================
    juce::UndoManager undoManager;
    ModulationManager modulationManager;
    LFOCurveState lfoCurveState;
    ModulateSynth synth;
    PeakMeter outputMeter { 2 };

    // The host's, kept from the last block that had one
    ModulateSynth::Tempo tempo;
    
    static juce::ValueTree getOrCreateStateChild(juce::ValueTree parent, const juce::Identifier& childID);

    // Points the modulation/LFO-curve managers at their state children. Shared by the
    // constructor and every state-restore path.
    void reinitialiseStateManagers();

    ModulateSynth::Settings currentSettings() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
