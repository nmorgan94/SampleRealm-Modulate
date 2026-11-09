#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "StepperDisplay.h"
#include "ComboChoices.h"
#include "../shared/ParameterSpecs.h"
#include "../dsp/WavetableGenerator.h"

// Forward declaration
class AudioPluginAudioProcessor;

class Oscillator : public juce::Component,
                   private juce::Timer,
                   private juce::Button::Listener
{
public:
    Oscillator(AudioPluginAudioProcessor& proc,
               const ParameterSpecs::Oscillator& specs,
               const juce::String& labelText)
        : processorRef(proc),
          waveformAID(specs.waveform.id),
          waveformBID(specs.waveformB.id),
          morphID(specs.morph.id),
          pitchParamIDStr(specs.pitch.id),
          gainSlider(proc, specs.gain.id),
          morphSlider(proc, specs.morph.id)
    {
        displaySourceA.setSize(1, WavetableGenerator::getWavetableSize());
        displaySourceB.setSize(1, WavetableGenerator::getWavetableSize());

        titleLabel.setText(labelText, juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel);
        
        enableButton.addListener(this);
        addAndMakeVisible(enableButton);
        
        addChoices(waveformSelector, specs.waveform);
        addChoices(waveformSelectorB, specs.waveformB);
        addAndMakeVisible(waveformSelector);
        addAndMakeVisible(waveformSelectorB);

        addAndMakeVisible(gainSlider);
        addAndMakeVisible(morphSlider);

        addAndMakeVisible(semitoneDisplay);
        semitoneDisplay.attachToParameter(proc, specs.pitch.id);

        fineDisplay.setLabel("FIN");
        addAndMakeVisible(fineDisplay);
        fineDisplay.attachToParameter(proc, specs.fine.id);

        auto& apvts = proc.getAPVTS();
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, specs.enable.id, enableButton);
        waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, specs.waveform.id, waveformSelector);
        waveformBAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, specs.waveformB.id, waveformSelectorB);
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, specs.gain.id, gainSlider);
        morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, specs.morph.id, morphSlider);

    }
    
    ~Oscillator() override
    {
        enableButton.removeListener(this);
        stopTimer();
    }
    
    void visibilityChanged() override
    {
        if (isVisible())
            startTimerHz(30);
        else
            stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        bool isEnabled = enableButton.getToggleState();
        float alpha = isEnabled ? 1.0f : 0.3f;
        
        titleLabel.setAlpha(alpha);
        waveformSelector.setAlpha(alpha);
        waveformSelector.setEnabled(isEnabled);
        waveformSelectorB.setAlpha(alpha);
        waveformSelectorB.setEnabled(isEnabled);
        gainSlider.setAlpha(alpha);
        gainSlider.setEnabled(isEnabled);
        morphSlider.setAlpha(alpha);
        morphSlider.setEnabled(isEnabled);
        semitoneDisplay.setAlpha(alpha);
        semitoneDisplay.setEnabled(isEnabled);
        fineDisplay.setAlpha(alpha);
        fineDisplay.setEnabled(isEnabled);
        
        // Draw background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        
        // Draw border
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
        
        auto waveformArea = bounds.reduced(10);
        waveformArea.removeFromTop(70);
        
        // Draw waveform background
        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.fillRoundedRectangle(waveformArea.toFloat(), 4.0f);
        
        drawWaveform(g, waveformArea.reduced(5), alpha);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        auto controlBar = bounds.removeFromTop(30);
        
        enableButton.setBounds(controlBar.removeFromLeft(30));
        
        controlBar.removeFromLeft(3);
        
        auto labelWidth = 68;
        titleLabel.setBounds(controlBar.removeFromLeft(labelWidth));
        
        controlBar.removeFromLeft(3);
        
        auto knobSize = 30;
        auto selectorWidth = controlBar.getWidth() - knobSize - 5;
        waveformSelector.setBounds(controlBar.removeFromLeft(selectorWidth));
        
        controlBar.removeFromLeft(5);
        
        gainSlider.setBounds(controlBar.removeFromLeft(knobSize).withHeight(knobSize));

        bounds.removeFromTop(5);
        auto morphRow = bounds.removeFromTop(30);

        auto gutter = morphRow.removeFromLeft(30 + 3 + labelWidth + 3);
        auto pitchWidth = 46;
        semitoneDisplay.setBounds(gutter.removeFromLeft(pitchWidth).withSizeKeepingCentre(pitchWidth, 20));
        gutter.removeFromLeft(6);
        auto fineWidth = 46;
        fineDisplay.setBounds(gutter.removeFromLeft(fineWidth).withSizeKeepingCentre(fineWidth, 20));

        auto morphSelectorWidth = morphRow.getWidth() - knobSize - 5;
        waveformSelectorB.setBounds(morphRow.removeFromLeft(morphSelectorWidth));
        morphRow.removeFromLeft(5);
        morphSlider.setBounds(morphRow.removeFromLeft(knobSize).withHeight(knobSize));
    }
    
    void timerCallback() override
    {
        repaint();
    }
    
    void buttonClicked(juce::Button*) override
    {
        repaint();
    }
    
    ModulatableSlider& getGainSlider() { return gainSlider; }

private:
    // Regenerate the cached display waveforms only when a waveform selection changes.
    // Runs on the message thread so the display never reads the audio thread's buffers.
    void updateDisplaySources()
    {
        auto& apvts = processorRef.getAPVTS();
        const int aIndex = static_cast<int>(apvts.getRawParameterValue(waveformAID)->load());
        if (aIndex != displayTypeA)
        {
            WavetableGenerator::generateWavetable(displaySourceA, static_cast<WaveformType>(aIndex));
            displayTypeA = aIndex;
        }

        const int bIndex = static_cast<int>(apvts.getRawParameterValue(waveformBID)->load());
        if (bIndex != displayTypeB)
        {
            WavetableGenerator::generateWavetable(displaySourceB, static_cast<WaveformType>(bIndex));
            displayTypeB = bIndex;
        }
    }

    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds, float alpha)
    {
        updateDisplaySources();

        const int numSamples = displaySourceA.getNumSamples();
        if (numSamples == 0)
            return;

        const float* samplesA = displaySourceA.getReadPointer(0);
        const float* samplesB = displaySourceB.getReadPointer(0);
        const float morph = juce::jlimit(0.0f, 1.0f, processorRef.getModulatedParam(morphID));

        juce::Path waveformPath;

        const float width = static_cast<float>(bounds.getWidth());
        const float height = static_cast<float>(bounds.getHeight());
        const float centerY = bounds.getY() + height / 2.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float sample = samplesA[i] + morph * (samplesB[i] - samplesA[i]);
            float x = bounds.getX() + (i / static_cast<float>(numSamples)) * width;
            float y = centerY - (sample * height * 0.4f);

            if (i == 0)
                waveformPath.startNewSubPath(x, y);
            else
                waveformPath.lineTo(x, y);
        }
        
        // Center line
        g.setColour(juce::Colours::grey.withAlpha(0.2f * alpha));
        g.drawLine(bounds.getX(), centerY, bounds.getRight(), centerY, 1.0f);
        
        g.setColour(juce::Colour(0xff00ff41).withAlpha(alpha));
        g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
        
        // Grid lines
        g.setColour(juce::Colours::grey.withAlpha(0.1f * alpha));
        for (int i = 1; i < 4; ++i)
        {
            float y = bounds.getY() + (i / 4.0f) * height;
            g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
        }
    }
    
    AudioPluginAudioProcessor& processorRef;
    juce::String waveformAID;
    juce::String waveformBID;
    juce::String morphID;
    juce::String pitchParamIDStr;
    
    juce::Label titleLabel;
    juce::ToggleButton enableButton;
    juce::ComboBox waveformSelector;
    juce::ComboBox waveformSelectorB;
    ModulatableSlider gainSlider;
    ModulatableSlider morphSlider;
    StepperDisplay semitoneDisplay;
    StepperDisplay fineDisplay;

    juce::AudioBuffer<float> displaySourceA;
    juce::AudioBuffer<float> displaySourceB;
    int displayTypeA = -1;
    int displayTypeB = -1;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformBAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oscillator)
};
