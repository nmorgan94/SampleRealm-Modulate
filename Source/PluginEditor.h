#pragma once

#include "PluginProcessor.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/Oscillator.h"
#include "ui/EnvelopePanel.h"
#include "ui/LFOPanel.h"
#include "ui/NoisePanel.h"
#include "ui/FilterPanel.h"
#include "ui/VoicePanel.h"
#include "ui/MasterMeter.h"
#include "ui/PresetBar.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::DragAndDropContainer
{
public:
    static constexpr int width = 900, height = 600;

    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Refreshes the UI caches the parameter attachments don't cover (modulation badges,
    // drawn LFO curves) after the PresetBar loads a preset. Lives here because it must
    // reach every panel/slider the editor owns.
    void refreshAfterPresetLoad();

    std::unique_ptr<Oscillator> osc1;
    std::unique_ptr<Oscillator> osc2;
    std::unique_ptr<Oscillator> osc3;
    
    std::unique_ptr<NoisePanel> noisePanel;
    std::unique_ptr<FilterPanel> filterPanel;
    std::unique_ptr<VoicePanel> voicePanel;
    std::unique_ptr<EnvelopePanel> envelopePanel;
    std::unique_ptr<LFOPanel> lfoPanel;
    std::unique_ptr<MasterMeter> masterMeter;

    std::unique_ptr<PresetBar> presetBar;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    CustomLookAndFeel customLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
