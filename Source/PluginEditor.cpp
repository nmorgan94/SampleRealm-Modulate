#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/ModulatableSlider.h"

namespace
{
    constexpr int topBarHeight   = 70;
    constexpr int edgeMargin     = 10;
    constexpr int titleWidth     = 550;
    constexpr int meterWidth     = 90;
    constexpr int presetBarWidth = 180;
    constexpr float titleHeight  = 52.0f;
    constexpr int versionHeight  = 12;
    constexpr int versionGap     = 2;
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    osc1 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, ParameterSpecs::osc[0], "OSC 1");
    addAndMakeVisible(osc1.get());

    osc2 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, ParameterSpecs::osc[1], "OSC 2");
    addAndMakeVisible(osc2.get());

    osc3 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, ParameterSpecs::osc[2], "OSC 3");
    addAndMakeVisible(osc3.get());
    
    noisePanel = std::make_unique<NoisePanel>(processorRef, processorRef.apvts);
    addAndMakeVisible(noisePanel.get());

    filterPanel = std::make_unique<FilterPanel>(processorRef, processorRef.apvts);
    addAndMakeVisible(filterPanel.get());

    voicePanel = std::make_unique<VoicePanel>(processorRef, processorRef.apvts);
    addAndMakeVisible(voicePanel.get());
    
    envelopePanel = std::make_unique<EnvelopePanel>(processorRef);
    addAndMakeVisible(envelopePanel.get());
    
    lfoPanel = std::make_unique<LFOPanel>(processorRef);
    addAndMakeVisible(lfoPanel.get());
    
    masterMeter = std::make_unique<MasterMeter>(processorRef.getOutputMeter(), processorRef.apvts);
    addAndMakeVisible(masterMeter.get());

    presetBar = std::make_unique<PresetBar>(processorRef);
    presetBar->onPresetLoaded = [this]() { refreshAfterPresetLoad(); };
    addAndMakeVisible(presetBar.get());

    setSize (width, height);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    
    auto titleArea = getLocalBounds().removeFromTop(topBarHeight)
                         .withTrimmedLeft(edgeMargin)
                         .withWidth(titleWidth);

    g.setColour (juce::Colour(0xff00ff00));
    g.setFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::boldWeight).withHeight(titleHeight));
    g.drawFittedText ("SampleRealm: MODULATE", titleArea, juce::Justification::centredLeft, 1);

    auto versionArea = titleArea.removeFromBottom(versionHeight).translated(0, versionGap);

    g.setColour (juce::Colour(0xff00ff41).withAlpha(0.4f));
    g.setFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::regularWeight).withPointHeight(9.0f));
    g.drawText ("v" + juce::String (JucePlugin_VersionString), versionArea,
                juce::Justification::centredLeft);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    auto topBar = bounds.removeFromTop(topBarHeight);
    topBar.removeFromRight(edgeMargin);
    if (masterMeter != nullptr)
        masterMeter->setBounds(topBar.removeFromRight(meterWidth).reduced(0, 6));

    topBar.removeFromLeft(edgeMargin + titleWidth);
    if (presetBar != nullptr)
        presetBar->setBounds(topBar.withSizeKeepingCentre(presetBarWidth, topBar.getHeight())
                                   .reduced(0, 20));
    
    // Padding
    bounds.reduce(10, 5);
    
    // Split into left (oscillators), middle (filter), and right (modulation) sections
    auto rightSection = bounds.removeFromRight(350);
    bounds.removeFromRight(10); 
    
    int filterWidth = 260;
    
    auto middleSection = bounds.removeFromRight(filterWidth);
    bounds.removeFromRight(10); // spacing
    
    // Left section: Stack oscillators vertically
    int oscHeight = (bounds.getHeight() - 20) / 3;
    
    osc1->setBounds(bounds.removeFromTop(oscHeight));
    bounds.removeFromTop(10);
    
    osc2->setBounds(bounds.removeFromTop(oscHeight));
    bounds.removeFromTop(10);
    
    osc3->setBounds(bounds);
    
    noisePanel->setBounds(middleSection.removeFromTop(50));
    middleSection.removeFromTop(10);

    int filterHeight = 260;
    filterPanel->setBounds(middleSection.removeFromTop(filterHeight));
    middleSection.removeFromTop(10);
    voicePanel->setBounds(middleSection);
    
    int panelHeight = (rightSection.getHeight() - 10) / 2;
    
    lfoPanel->setBounds(rightSection.removeFromTop(panelHeight));
    rightSection.removeFromTop(10);

    envelopePanel->setBounds(rightSection);
}

//==============================================================================
namespace
{
    // Depth-first walk that re-reads each modulation slider's assignment from the manager.
    void refreshModulatableSliders(juce::Component* component)
    {
        for (auto* child : component->getChildren())
        {
            if (auto* slider = dynamic_cast<ModulatableSlider*>(child))
                slider->refreshAssignment();

            refreshModulatableSliders(child);
        }
    }
}

void AudioPluginAudioProcessorEditor::refreshAfterPresetLoad()
{
    // APVTS-attached knobs/menus follow the new parameter values automatically. The two UI
    // caches that don't are the modulation badges and the drawn LFO curves - refresh those.
    refreshModulatableSliders(this);

    if (lfoPanel != nullptr)
        lfoPanel->reloadFromState();
}
