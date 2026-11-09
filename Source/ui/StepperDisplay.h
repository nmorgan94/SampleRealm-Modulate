#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../ModulationManager.h"
#include "CustomLookAndFeel.h"

class AudioPluginAudioProcessor;

class StepperDisplay : public juce::Component,
                       public juce::DragAndDropTarget,
                       private juce::Timer
{
public:
    StepperDisplay() = default;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

        if (isModulated())
            bounds.removeFromTop(7);

        auto labelBounds = bounds.removeFromLeft(bounds.getWidth() / 2);

        g.setColour(juce::Colours::grey);
        g.setFont(CustomLookAndFeel::orbitron(CustomLookAndFeel::boldWeight).withHeight(10.0f));
        g.drawText(labelText, labelBounds, juce::Justification::centred);

        g.setColour(juce::Colours::white);
        g.setFont(CustomLookAndFeel::orbitron(CustomLookAndFeel::boldWeight).withHeight(12.0f));
        juce::String valueText = (showSign && value >= 0 ? "+" : "") + juce::String(value);
        g.drawText(valueText, bounds, juce::Justification::centred);

        if (isModulated())
        {
            drawModulationBar(g);
            drawModulationBadge(g);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (isModulated())
        {
            if (getBadgeBounds().contains(e.getPosition()))
            {
                if (e.mods.isRightButtonDown())
                {
                    showModulationMenu();
                    return;
                }
                if (e.mods.isLeftButtonDown())
                {
                    isDraggingModulation = true;
                    modDragStart = e.getPosition();
                    return;
                }
            }
            else if (e.mods.isRightButtonDown())
            {
                showModulationMenu();
                return;
            }
        }

        dragStartY = e.y;
        dragStartValue = value;
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        setValue(juce::jlimit(minValue, maxValue, 0));
        if (onValueChange)
            onValueChange(value);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isDraggingModulation)
        {
            int dragDistance = e.getPosition().y - modDragStart.y;
            float delta = -dragDistance * 0.003f;

            auto assignment = getModulationManager().getAssignment(parameterID);
            Modulation::applyDepthChange(assignment, normalizedBase(), delta);
            applyRange(assignment);
            repaint();
            return;
        }

        int deltaY = dragStartY - e.y;
        int newValue = juce::jlimit(minValue, maxValue, dragStartValue + (deltaY / 3));

        if (newValue != value)
        {
            value = newValue;
            repaint();

            if (onValueChange)
                onValueChange(value);
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isDraggingModulation = false;
    }

    void setValue(int newValue)
    {
        value = juce::jlimit(minValue, maxValue, newValue);
        repaint();
    }

    int getValue() const { return value; }

    void setRange(int newMin, int newMax)
    {
        minValue = newMin;
        maxValue = newMax;
        setValue(value);
    }

    void setLabel(const juce::String& newLabel)
    {
        labelText = newLabel;
        repaint();
    }

    void setShowSign(bool shouldShowSign)
    {
        showSign = shouldShowSign;
        repaint();
    }

    // Two-way binding to an integer APVTS parameter: initialises from the current
    // value and writes changes back to the host.
    void attachToParameter(AudioPluginAudioProcessor& proc, const juce::String& paramID)
    {
        processor = &proc;
        parameterID = paramID;

        if (auto* param = dynamic_cast<juce::AudioParameterInt*>(getParameter()))
        {
            const auto range = param->getNormalisableRange();
            setRange(juce::roundToInt(range.start), juce::roundToInt(range.end));
            setValue(param->get());
        }

        onValueChange = [this](int newValue)
        {
            if (auto* param = getParameter())
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(newValue)));

            updateModulationRangeFromValue();
        };

        restoreAssignment();
    }

    std::function<void(int)> onValueChange;

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

    void visibilityChanged() override
    {
        updateTimerState();
    }

private:
    static inline const juce::Colour modulationColor = juce::Colour(0xff00ff41);  // Neon green

    int value = 0;
    int minValue = -24;
    int maxValue = 24;
    bool showSign = true;
    juce::String labelText { "SEM" };
    int dragStartY = 0;
    int dragStartValue = 0;

    AudioPluginAudioProcessor* processor = nullptr;
    juce::String parameterID;
    int assignedSourceIndex = -1;
    bool assignedIsLFO = true;
    bool isDraggingModulation = false;
    juce::Point<int> modDragStart;

    bool isModulated() const { return assignedSourceIndex >= 0 && assignedSourceIndex < 4; }

    ModulationManager& getModulationManager()
    {
        return processor->getModulationManager();
    }

    juce::RangedAudioParameter* getParameter()
    {
        return dynamic_cast<juce::RangedAudioParameter*>(processor->getAPVTS().getParameter(parameterID));
    }

    float getModulatedValueNormalized()
    {
        if (auto* param = getParameter())
            return param->getNormalisableRange().convertTo0to1(processor->getModulatedParam(parameterID));
        return 0.0f;
    }

    float normalizedBase()
    {
        if (auto* param = getParameter())
            return param->getNormalisableRange().convertTo0to1(static_cast<float>(value));
        return 0.0f;
    }

    juce::Rectangle<int> getBadgeBounds() const
    {
        return getLocalBounds().removeFromTop(9).removeFromRight(15).translated(-1, -1);
    }

    void updateTimerState()
    {
        if (isVisible() && isModulated())
            startTimerHz(30);
        else
            stopTimer();
    }

    void timerCallback() override
    {
        if (isModulated())
            repaint();
    }

    void setAssignedSource(bool isLFO, int sourceIndex)
    {
        assignedIsLFO = isLFO;
        assignedSourceIndex = sourceIndex;
        updateTimerState();
        repaint();
    }

    void applyRange(const Modulation::Assignment& assignment)
    {
        getModulationManager().setAssignment(parameterID, assignment);
    }

    void updateModulationRangeFromValue()
    {
        if (!isModulated() || processor == nullptr)
            return;

        auto assignment = getModulationManager().getAssignment(parameterID);
        Modulation::applyDepthChange(assignment, normalizedBase(), 0.0f);
        applyRange(assignment);
    }

    void assignModulator(bool isLFO, int index)
    {
        if (processor != nullptr && getParameter() != nullptr)
        {
            auto type = isLFO ? Modulation::Source::LFO
                              : Modulation::Source::Envelope;
            applyRange(Modulation::makeDefaultRange(type, index, normalizedBase()));
        }

        setAssignedSource(isLFO, index);
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

    void clearAssignment()
    {
        getModulationManager().clearAssignment(parameterID);
        setAssignedSource(true, -1);
        repaint();
    }

    void restoreAssignment()
    {
        auto assignment = getModulationManager().getAssignment(parameterID);
        if (assignment.isAssigned())
            setAssignedSource(assignment.isLFO(), assignment.sourceIndex);
    }

    juce::Rectangle<float> getModulationTrack() const
    {
        // A thin strip along the top edge
        auto bounds = getLocalBounds().toFloat();
        const float leftPad = 4.0f, topPad = 1.0f, trackH = 3.0f, gap = 2.0f;
        float rightLimit = static_cast<float>(getBadgeBounds().getX()) - gap;
        return { bounds.getX() + leftPad, bounds.getY() + topPad,
                 juce::jmax(1.0f, rightLimit - (bounds.getX() + leftPad)), trackH };
    }

    void drawModulationBar(juce::Graphics& g)
    {
        auto* param = getParameter();
        if (!param) return;

        auto track = getModulationTrack();
        auto assignment = getModulationManager().getAssignment(parameterID);

        // Track backgroundx
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(track, track.getHeight() * 0.5f);

        // Modulated range span
        float minX = track.getX() + assignment.minValue * track.getWidth();
        float maxX = track.getX() + assignment.maxValue * track.getWidth();
        juce::Rectangle<float> span(minX, track.getY(), juce::jmax(1.0f, maxX - minX), track.getHeight());
        g.setColour(modulationColor.withAlpha(0.5f));
        g.fillRoundedRectangle(span, track.getHeight() * 0.5f);

        // Live indicator
        float currentMod = getModulatedValueNormalized();
        float x = track.getX() + currentMod * track.getWidth();
        float cy = track.getCentreY();
        g.setColour(modulationColor.withAlpha(0.9f));
        g.fillEllipse(x - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillEllipse(x - 1.25f, cy - 1.25f, 2.5f, 2.5f);
    }

    void drawModulationBadge(juce::Graphics& g)
    {
        auto indicator = getBadgeBounds().toFloat();
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(indicator, 2.0f);
        g.setColour(modulationColor);
        g.setFont(CustomLookAndFeel::orbitron(CustomLookAndFeel::regularWeight).withHeight(9.0f));
        g.drawText((assignedIsLFO ? "L" : "E") + juce::String(assignedSourceIndex + 1),
                   indicator, juce::Justification::centred);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepperDisplay)
};
