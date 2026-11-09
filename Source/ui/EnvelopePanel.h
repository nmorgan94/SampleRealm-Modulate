#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "EnvelopeEditor.h"
#include "CustomLFOTabButton.h"
#include "CustomLookAndFeel.h"
#include "ValueLabelSlider.h"

class AudioPluginAudioProcessor;

class CustomEnvelopeTabbedComponent : public juce::TabbedComponent
{
public:
    CustomEnvelopeTabbedComponent() : juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop)
    {
    }
    
    juce::TabBarButton* createTabButton(const juce::String& tabName, int tabIndex) override
    {
        return new CustomLFOTabButton(tabName, getTabbedButtonBar(), tabIndex, CustomLFOTabButton::SourceType::Envelope);
    }
};

class EnvelopeContentPanel : public juce::Component,
                             public juce::Slider::Listener
{
public:
    EnvelopeContentPanel(AudioPluginAudioProcessor& proc, int envIndex)
        : processor(proc)
    {
        // Create envelope editor
        envelopeEditor = std::make_unique<EnvelopeEditor>();
        envelopeEditor->setEnvelope(&processor.getLive(),
                                    &processor.getModulationManager(), envIndex);
        addAndMakeVisible(envelopeEditor.get());
        
        // Create ADSR sliders
        const auto& specs = ParameterSpecs::env[static_cast<size_t>(envIndex)];

        setupSlider(attackSlider, attackLabel, "Attack", specs.attack.id);
        setupSlider(decaySlider, decayLabel, "Decay", specs.decay.id);
        setupSlider(sustainSlider, sustainLabel, "Sustain", specs.sustain.id);
        setupSlider(releaseSlider, releaseLabel, "Release", specs.release.id);
        
        // Initial update
        updateEnvelopeDisplay();
    }
    
    ~EnvelopeContentPanel() override
    {
        attackSlider.removeListener(this);
        decaySlider.removeListener(this);
        sustainSlider.removeListener(this);
        releaseSlider.removeListener(this);
    }
    
    void sliderValueChanged(juce::Slider* slider) override
    {
        juce::ignoreUnused(slider);
        updateEnvelopeDisplay();
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Envelope editor takes top portion
        int editorHeight = juce::roundToInt(bounds.getHeight() * 0.65f);
        auto editorBounds = bounds.removeFromTop(editorHeight);
        envelopeEditor->setBounds(editorBounds);
        
        bounds.removeFromTop(10); // Spacing
        
        auto knobSize = 50;
        auto spacing = (bounds.getWidth() - (knobSize * 4)) / 5;
        
        juce::Slider* sliders[] = { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider };
        juce::Label* labels[] = { &attackLabel, &decayLabel, &sustainLabel, &releaseLabel };
        
        for (int i = 0; i < 4; ++i)
        {
            bounds.removeFromLeft(spacing);
            auto knobBounds = bounds.removeFromLeft(knobSize);
            labels[i]->setBounds(knobBounds.removeFromTop(16));
            sliders[i]->setBounds(knobBounds.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize));
        }
    }

private:
    void setupSlider(ValueLabelSlider& slider, juce::Label& label,
                     const juce::String& labelText, const juce::String& paramID)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.addListener(this);
        addAndMakeVisible(slider);
        
        auto* param = processor.getAPVTS().getParameter(paramID);
        if (param != nullptr)
        {
            sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getAPVTS(), paramID, slider));
        }
        
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
        addAndMakeVisible(label);

        slider.attachLabel(label);
    }
    
    void updateEnvelopeDisplay()
    {
        auto attack = attackSlider.getValue();
        auto decay = decaySlider.getValue();
        auto sustain = sustainSlider.getValue();
        auto release = releaseSlider.getValue();
        
        envelopeEditor->setADSRValues(attack, decay, sustain, release);
    }
    
    AudioPluginAudioProcessor& processor;
    
    std::unique_ptr<EnvelopeEditor> envelopeEditor;
    
    ValueLabelSlider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeContentPanel)
};

class EnvelopePanel : public juce::Component
{
public:
    EnvelopePanel(AudioPluginAudioProcessor& proc) : processor(proc)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto panel = std::make_unique<EnvelopeContentPanel>(processor, i);
            envelopePanels.push_back(std::move(panel));
            
            tabbedComponent.addTab("ENV " + juce::String(i + 1),
                                  juce::Colour(0xff1a1a1a),
                                  envelopePanels.back().get(),
                                  false);
        }
        
        tabbedComponent.setTabBarDepth(40);
        tabbedComponent.setOutline(0);
        
        addAndMakeVisible(tabbedComponent);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);
    }
    
    void resized() override
    {
        tabbedComponent.setBounds(getLocalBounds());
    }

private:
    AudioPluginAudioProcessor& processor;
    CustomEnvelopeTabbedComponent tabbedComponent;
    std::vector<std::unique_ptr<EnvelopeContentPanel>> envelopePanels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopePanel)
};
