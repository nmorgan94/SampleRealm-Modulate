#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "EnvelopeEditor.h"
#include "CustomLFOTabButton.h"
#include "CustomLookAndFeel.h"
#include "LabeledKnob.h"

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

        setupKnob(attackKnob, "Attack", specs.attack.id);
        setupKnob(decayKnob, "Decay", specs.decay.id);
        setupKnob(sustainKnob, "Sustain", specs.sustain.id);
        setupKnob(releaseKnob, "Release", specs.release.id);

        auto& apvts = processor.getAPVTS();
        envelopeEditor->setCurveParameters({ apvts.getParameter(specs.attackCurve.id),
                                             apvts.getParameter(specs.decayCurve.id),
                                             apvts.getParameter(specs.releaseCurve.id) });
        
        // Initial update
        updateEnvelopeDisplay();
    }
    
    ~EnvelopeContentPanel() override
    {
        for (auto* knob : knobs())
            knob->getSlider().removeListener(this);
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
        
        for (auto* knob : knobs())
        {
            bounds.removeFromLeft(spacing);
            knob->setBounds(bounds.removeFromLeft(knobSize).withHeight(LabeledKnob<>::labelHeight + knobSize));
        }
    }

private:
    std::array<LabeledKnob<>*, 4> knobs() { return { &attackKnob, &decayKnob, &sustainKnob, &releaseKnob }; }

    void setupKnob(LabeledKnob<>& knob, const juce::String& labelText, const juce::String& paramID)
    {
        knob.setLabelText(labelText);
        knob.setLabelColour(juce::Colour(0xff00ff41));
        knob.getSlider().addListener(this);
        addAndMakeVisible(knob);
        
        auto* param = processor.getAPVTS().getParameter(paramID);
        if (param != nullptr)
        {
            sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getAPVTS(), paramID, knob.getSlider()));
        }
    }
    
    void updateEnvelopeDisplay()
    {
        auto attack = attackKnob.getSlider().getValue();
        auto decay = decayKnob.getSlider().getValue();
        auto sustain = sustainKnob.getSlider().getValue();
        auto release = releaseKnob.getSlider().getValue();
        
        envelopeEditor->setADSRValues(attack, decay, sustain, release);
    }
    
    AudioPluginAudioProcessor& processor;
    
    std::unique_ptr<EnvelopeEditor> envelopeEditor;
    
    LabeledKnob<> attackKnob, decayKnob, sustainKnob, releaseKnob;
    
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
