#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "CustomLookAndFeel.h"
#include "ValueLabelSlider.h"

// Forward declaration
class AudioPluginAudioProcessor;

class ModulatableSlider : public ValueLabelSlider,
                          public juce::DragAndDropTarget,
                          private juce::Timer
{
public:
    ModulatableSlider(AudioPluginAudioProcessor& proc, const juce::String& paramID)
        : processor(proc), parameterID(paramID)
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        refreshAssignment();

        onValueChange = [this]() { updateModulationRangeFromSlider(); };
    }

    const juce::String& getParameterID() const { return parameterID; }

    void setParameterID(const juce::String& newParameterID)
    {
        parameterID = newParameterID;
        refreshAssignment();
    }

    // Re-reads this parameter's modulation assignment from the manager and updates the
    // badge/arc — including clearing it when the (e.g. newly loaded) preset has none.
    // Called on construction and after a preset load.
    void refreshAssignment()
    {
        auto assignment = processor.getModulationManager().getAssignment(parameterID);
        if (assignment.isAssigned())
            setAssignedSource(assignment.isLFO(), assignment.sourceIndex);
        else
            setAssignedSource(true, -1);
    }
    
    void visibilityChanged() override
    {
        updateTimerState();
    }
    
    void setAssignedSource(bool isLFO, int sourceIndex)
    {
        assignedIsLFO = isLFO;
        assignedSourceIndex = sourceIndex;
        updateTimerState();
        repaint();
    }
    
    // DragAndDropTarget overrides
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        auto desc = dragSourceDetails.description.toString();
        return desc.startsWith("LFO:") || desc.startsWith("ENV:");
    }
    
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        auto desc = dragSourceDetails.description.toString();
        bool isLFO = desc.startsWith("LFO:");
        int index = desc.substring(4).getIntValue();
        
        if (index >= 0 && index < 4)
            assignModulator(isLFO, index);
    }
    
    void paint(juce::Graphics& g) override
    {
        juce::Slider::paint(g);
        
        if (assignedSourceIndex >= 0 && assignedSourceIndex < 4)
        {
            drawModulationArc(g);
            drawModulationBadge(g);
        }
    }
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (assignedSourceIndex >= 0)
        {
            auto indicator = getLocalBounds().removeFromTop(12).removeFromRight(16).translated(-2, 2);
            
            if (indicator.contains(event.getPosition()))
            {
                if (event.mods.isRightButtonDown())
                {
                    showModulationMenu();
                    return;
                }
                else if (event.mods.isLeftButtonDown())
                {
                    isDraggingModulation = true;
                    modDragStart = event.getPosition();
                    return;
                }
            }
            else if (event.mods.isRightButtonDown())
            {
                showModulationMenu();
                return;
            }
        }
        
        juce::Slider::mouseDown(event);
    }
    
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (isDraggingModulation)
        {
            int dragDistance = event.getPosition().y - modDragStart.y;
            float delta = -dragDistance * 0.003f;
            
            if (auto* param = getParameter())
            {
                float normalizedBase = param->getNormalisableRange().convertTo0to1(static_cast<float>(getValue()));

                auto assignment = processor.getModulationManager().getAssignment(parameterID);
                Modulation::applyDepthChange(assignment, normalizedBase, delta);
                processor.getModulationManager().setAssignment(parameterID, assignment);

                repaint();
            }
        }
        else
        {
            juce::Slider::mouseDrag(event);
        }
    }
    
    void mouseUp(const juce::MouseEvent& event) override
    {
        isDraggingModulation = false;
        juce::Slider::mouseUp(event);
    }

private:
    static inline const juce::Colour modulationColor = juce::Colour(0xff00ff41);  // Neon green for all modulation
    
    AudioPluginAudioProcessor& processor;
    juce::String parameterID;
    int assignedSourceIndex = -1;
    bool assignedIsLFO = true;
    bool isDraggingModulation = false;
    juce::Point<int> modDragStart;
    
    void updateTimerState()
    {
        if (isVisible() && assignedSourceIndex >= 0)
            startTimerHz(30);
        else
            stopTimer();
    }
    
    void timerCallback() override
    {
        if (assignedSourceIndex >= 0)
            repaint();
    }
    
    juce::RangedAudioParameter* getParameter()
    {
        return processor.getAPVTS().getParameter(parameterID);
    }
    
    void updateModulationRangeFromSlider()
    {
        if (assignedSourceIndex < 0)
            return;

        if (auto* param = getParameter())
        {
            float normalizedBase = param->getNormalisableRange().convertTo0to1(static_cast<float>(getValue()));

            auto assignment = processor.getModulationManager().getAssignment(parameterID);
            Modulation::applyDepthChange(assignment, normalizedBase, 0.0f);
            processor.getModulationManager().setAssignment(parameterID, assignment);
        }
    }

    void assignModulator(bool isLFO, int index)
    {
        if (auto* param = getParameter())
        {
            float normalizedBase = param->getNormalisableRange().convertTo0to1(static_cast<float>(getValue()));
            auto type = isLFO ? Modulation::Source::LFO
                              : Modulation::Source::Envelope;
            processor.getModulationManager().setAssignment(parameterID,
                Modulation::makeDefaultRange(type, index, normalizedBase));
        }

        setAssignedSource(isLFO, index);
        DBG((isLFO ? "LFO " : "Envelope ") << (index + 1) << " assigned to " << parameterID);
    }
    
    void showModulationMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Clear Assignment");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result == 1)
                clearAssignment();
        });
    }
    void drawModulationArc(juce::Graphics& g)
    {
        auto* param = getParameter();
        if (!param) return;
        
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        float sliderRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float trackWidth = sliderRadius * 0.15f;
        float arcRadius = sliderRadius - trackWidth * 0.5f;
        
        auto rotaryParams = getRotaryParameters();
        float angleRange = rotaryParams.endAngleRadians - rotaryParams.startAngleRadians;
        auto assignment = processor.getModulationManager().getAssignment(parameterID);
        
        // Draw arc
        float minAngle = rotaryParams.startAngleRadians + (assignment.minValue * angleRange);
        float maxAngle = rotaryParams.startAngleRadians + (assignment.maxValue * angleRange);
        juce::Path arcPath;
        arcPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, minAngle, maxAngle, true);
        g.setColour(modulationColor.withAlpha(0.5f));
        g.strokePath(arcPath, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        // Draw animated indicator
        float currentMod = processor.getModulatedParam(parameterID);
        float currentAngle = rotaryParams.startAngleRadians + (param->getNormalisableRange().convertTo0to1(currentMod) * angleRange);
        float x = centre.x + arcRadius * std::cos(currentAngle - juce::MathConstants<float>::halfPi);
        float y = centre.y + arcRadius * std::sin(currentAngle - juce::MathConstants<float>::halfPi);
        
        g.setColour(modulationColor.withAlpha(0.7f));
        g.fillEllipse(x - 3, y - 3, 6, 6);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillEllipse(x - 1.5f, y - 1.5f, 3, 3);
    }
    
    void drawModulationBadge(juce::Graphics& g)
    {
        auto indicator = getLocalBounds().removeFromTop(12).removeFromRight(16).translated(-2, 2).toFloat();
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(indicator, 2.0f);
        g.setColour(modulationColor);
        g.setFont(CustomLookAndFeel::orbitron(CustomLookAndFeel::regularWeight).withHeight(9.0f));
        g.drawText((assignedIsLFO ? "L" : "E") + juce::String(assignedSourceIndex + 1),
                  indicator, juce::Justification::centred);
    }
    
    
    void clearAssignment()
    {
        processor.getModulationManager().clearAssignment(parameterID);
        setAssignedSource(true, -1);
        repaint();
        DBG("Modulation assignment cleared from parameter: " << parameterID);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatableSlider)
};
