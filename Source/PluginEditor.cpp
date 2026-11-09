#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/ModulatableSlider.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(&customLookAndFeel);

    addAndMakeVisible(header);

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
    
    prompt.setFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::regularWeight).withHeight (14.0f));
    addChildComponent(prompt);
    addChildComponent(manual);

    setSize (width, height);
    startTimerHz (10);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    header.setBounds(bounds.removeFromTop(headerLayout.height));
    
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

void AudioPluginAudioProcessorEditor::timerCallback()
{
    if (const auto change = processorRef.getStateChanges(); change != lastStateChange)
    {
        lastStateChange = change;
        refreshAfterStateChange();
    }
}

void AudioPluginAudioProcessorEditor::refreshAfterStateChange()
{
    // APVTS-attached knobs/menus follow the new parameter values automatically. The two UI
    // caches that don't are the modulation badges and the drawn LFO curves - refresh those.
    refreshModulatableSliders(this);

    if (lfoPanel != nullptr)
        lfoPanel->reloadFromState();
}
