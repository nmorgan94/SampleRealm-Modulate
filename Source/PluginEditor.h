#pragma once

#include "PluginProcessor.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/Oscillator.h"
#include "ui/EnvelopePanel.h"
#include "ui/LFOPanel.h"
#include "ui/NoisePanel.h"
#include "ui/FilterPanel.h"
#include "ui/VoicePanel.h"
#include "ui/About.h"
#include "ui/HeaderBar.h"
#include "ui/Manual.h"
#include "ui/PromptOverlay.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::DragAndDropContainer,
                                               private juce::Timer
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
    // drawn LFO curves) after the state is replaced. Lives here because it must
    // reach every panel/slider the editor owns.
    void refreshAfterStateChange();

    void timerCallback() override;

    std::unique_ptr<Oscillator> osc1;
    std::unique_ptr<Oscillator> osc2;
    std::unique_ptr<Oscillator> osc3;
    
    std::unique_ptr<NoisePanel> noisePanel;
    std::unique_ptr<FilterPanel> filterPanel;
    std::unique_ptr<VoicePanel> voicePanel;
    std::unique_ptr<EnvelopePanel> envelopePanel;
    std::unique_ptr<LFOPanel> lfoPanel;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    CustomLookAndFeel customLookAndFeel;

    PromptOverlay prompt;
    ManualOverlay manual;
    HeaderBar header { "SampleRealm: MODULATE", processorRef.getPresetManager(), prompt, [this] { manual.show(); },
                       About::text (processorRef), processorRef.getOutputMeter(), processorRef.getAPVTS() };

    juce::uint32 lastStateChange = processorRef.getStateChanges();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
