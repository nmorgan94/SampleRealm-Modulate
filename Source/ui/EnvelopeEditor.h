#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BaseCurveDisplay.h"
#include "CustomLookAndFeel.h"
#include "../ModulationManager.h"

class EnvelopeEditor : public BaseCurveDisplay,
                       public juce::Timer
{
public:
    EnvelopeEditor() = default;

    ~EnvelopeEditor() override
    {
        stopTimer();
    }

    void setEnvelope(const ModulateSynth::Live* liveState, const ModulationManager* mod, int index)
    {
        live = liveState;
        modManager = mod;
        envelopeIndex = index;
        updateTimerState();
    }

    void visibilityChanged() override
    {
        updateTimerState();
    }

    void updateTimerState()
    {
        if (isVisible() && live != nullptr)
            startTimerHz(60);
        else
            stopTimer();
    }

    void timerCallback() override
    {
        const bool visible = isPlayheadVisible();
        if (visible || playheadWasVisible)
            repaint();
        playheadWasVisible = visible;
    }

    void setADSRValues(double attackTime, double decayTime, double sustainLevel, double releaseTime)
    {
        attack = juce::jmax(0.001f, float(attackTime));
        decay = juce::jmax(0.001f, float(decayTime));
        sustain = juce::jlimit(0.0f, 1.0f, float(sustainLevel));
        release = juce::jmax(0.001f, float(releaseTime));
        repaint();
    }

protected:
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
    {
        const auto stage = getStageBounds(bounds);

        const float topY     = bounds.getY();
        const float bottomY  = bounds.getBottom();
        const float sustainY = bottomY - (sustain * bounds.getHeight());

        path.startNewSubPath(stage.startX, bottomY); // start at bottom left
        path.lineTo(stage.attackX,  topY);
        path.lineTo(stage.decayX,   sustainY);
        path.lineTo(stage.sustainX, sustainY);
        path.lineTo(stage.releaseX, bottomY);
    }

    void drawControlPoints(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        const auto stage = getStageBounds(bounds);

        const float sustainY = bounds.getBottom() - (sustain * bounds.getHeight());

        drawPoint(g, stage.attackX,  bounds.getY());
        drawPoint(g, stage.decayX,   sustainY);
        drawPoint(g, stage.sustainX, sustainY);

        if (isPlayheadVisible())
        {
            const float playheadX = getPlayheadX(stage);
            g.setColour(juce::Colour(0xff00ff41).brighter(0.5f));
            g.drawLine(playheadX, bounds.getY(), playheadX, bounds.getBottom(), 2.0f);
        }
    }

    void drawLabels(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.6f));
        g.setFont(CustomLookAndFeel::orbitron(CustomLookAndFeel::regularWeight).withHeight(10.0f));

        const auto stage = getStageBounds(bounds);

        // Draw a stage label
        auto drawLabel = [&](const juce::String& text, float x1, float x2)
        {
            const float centerX = (x1 + x2) / 2.0f;
            const float labelY   = bounds.getBottom() - 15.0f;
            juce::Rectangle<float> labelBounds(centerX - 20.0f, labelY, 40.0f, 12.0f);
            g.drawText(text, labelBounds, juce::Justification::centred);
        };

        drawLabel("A", stage.startX,   stage.attackX);
        drawLabel("D", stage.attackX,  stage.decayX);
        drawLabel("S", stage.decayX,   stage.sustainX);
        drawLabel("R", stage.sustainX, stage.releaseX);
    }

private:
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;

    static constexpr float sustainDisplayTime = 0.3f;

    struct StageBounds { float startX, attackX, decayX, sustainX, releaseX; };

    StageBounds getStageBounds(const juce::Rectangle<float>& bounds) const
    {
        const float totalTime = attack + decay + sustainDisplayTime + release;
        const float w = bounds.getWidth();
        const float x = bounds.getX();

        StageBounds s;
        s.startX   = x;
        s.attackX  = x + (attack / totalTime) * w;
        s.decayX   = s.attackX + (decay / totalTime) * w;
        s.sustainX = s.decayX + (sustainDisplayTime / totalTime) * w;
        s.releaseX = x + w;
        return s;
    }

    float getPlayheadX(const StageBounds& s) const
    {
        const float p = live->envelopeProgress[liveIndex()];
        switch (live->envelopeStage[liveIndex()].load())
        {
            case ADSREnvelope::State::Attack:  return juce::jmap(p, s.startX,   s.attackX);
            case ADSREnvelope::State::Decay:   return juce::jmap(p, s.attackX,  s.decayX);
            case ADSREnvelope::State::Sustain: return s.decayX; // park at start of the sustain plateau
            case ADSREnvelope::State::Release: return juce::jmap(p, s.sustainX, s.releaseX);
            default:                           return s.startX;
        }
    }

    bool isPlayheadVisible() const
    {
        if (live == nullptr || live->envelopeStage[liveIndex()] == ADSREnvelope::State::Idle)
            return false;

        return (envelopeIndex == 0)
            || (modManager != nullptr && Modulation::isSourceAssigned(modManager->getAssignments(), Modulation::Source::Envelope, envelopeIndex));
    }

    size_t liveIndex() const { return static_cast<size_t>(envelopeIndex); }

    const ModulateSynth::Live* live = nullptr;
    const ModulationManager* modManager = nullptr;
    int envelopeIndex = -1;
    bool playheadWasVisible = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeEditor)
};
