#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BaseCurveDisplay.h"
#include "CustomLookAndFeel.h"
#include "../ModulationManager.h"
#include "../dsp/EnvelopeCurve.h"

class EnvelopeEditor : public BaseCurveDisplay,
                       public juce::Timer
{
public:
    static constexpr size_t numSegments = 3;

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

    // Attack, decay and release
    void setCurveParameters(std::array<juce::RangedAudioParameter*, numSegments> parameters)
    {
        for (size_t i = 0; i < numSegments; ++i)
        {
            curveAttachments[i] = std::make_unique<juce::ParameterAttachment>(*parameters[i], [this, i](float value)
            {
                curves[i] = value;
                repaint();
            });
            curveAttachments[i]->sendInitialUpdate();
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        setHovered(findHandle(e.position));
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (! dragged)
            setHovered({});
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto handle = findHandle(e.position);
        if (! handle || ! e.mods.isLeftButtonDown() || e.getNumberOfClicks() > 1)
            return;

        dragged = handle;
        dragStartCurve = curves[*handle];
        curveAttachments[*handle]->beginGesture();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (! dragged)
            return;

        const float dy = e.position.y - e.mouseDownPosition.y;
        curveAttachments[*dragged]->setValueAsPartOfGesture(dragStartCurve + dy * dragDirections[*dragged] * curvePerPixel);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (! dragged)
            return;

        curveAttachments[*dragged]->endGesture();
        dragged.reset();
        setHovered(findHandle(e.position));
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (const auto handle = findHandle(e.position))
            curveAttachments[*handle]->setValueAsCompleteGesture(0.0f);
    }

protected:
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
    {
        path.startNewSubPath(bounds.getBottomLeft());

        for (const auto& segment : getSegments(bounds))
            for (int i = 0; i <= segmentSteps; ++i)
                path.lineTo(segment.pointAt(float(i) / segmentSteps));
    }

    void drawControlPoints(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        const auto segments = getSegments(bounds);

        for (const auto point : { segments[0].to, segments[1].to, segments[2].from })
            drawPoint(g, point.x, point.y);

        const auto active = dragged ? dragged : hovered;
        for (size_t i = 0; i < numSegments; ++i)
        {
            const bool isActive = active == i;
            const float radius = isActive ? handleRadius + 1.5f : handleRadius;
            g.setColour(juce::Colour(0xff00ff41).withAlpha(isActive ? 1.0f : 0.5f));
            g.fillEllipse(juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(segments[i].pointAt(0.5f)));
        }

        if (isPlayheadVisible())
        {
            const float playheadX = getPlayheadX(getStageBounds(bounds));
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
    static constexpr int segmentSteps = 32;
    static constexpr float handleRadius = 3.0f;
    static constexpr float hitRadius = 8.0f;
    static constexpr float curvePerPixel = 1.0f / 120.0f;

    std::array<float, numSegments> curves {};
    std::array<std::unique_ptr<juce::ParameterAttachment>, numSegments> curveAttachments;
    std::optional<size_t> hovered, dragged;
    float dragStartCurve = 0.0f;

    // Towards each segment's end
    static constexpr std::array dragDirections { -1.0f, 1.0f, 1.0f };

    struct Segment
    {
        juce::Point<float> from, to;
        float curve;

        juce::Point<float> pointAt(float t) const
        {
            return { juce::jmap(t, from.x, to.x), juce::jmap(EnvelopeCurve::shapeProgress(t, curve), from.y, to.y) };
        }
    };

    std::array<Segment, numSegments> getSegments(const juce::Rectangle<float>& bounds) const
    {
        const auto stage = getStageBounds(bounds);
        const float sustainY = bounds.getBottom() - (sustain * bounds.getHeight());

        return {{
            { { stage.startX, bounds.getBottom() }, { stage.attackX, bounds.getY() }, curves[0] },
            { { stage.attackX, bounds.getY() }, { stage.decayX, sustainY }, curves[1] },
            { { stage.sustainX, sustainY }, { stage.releaseX, bounds.getBottom() }, curves[2] }
        }};
    }

    std::optional<size_t> findHandle(juce::Point<float> position) const
    {
        const auto segments = getSegments(getLocalBounds().toFloat());
        std::optional<size_t> found;
        float closest = hitRadius;

        for (size_t i = 0; i < numSegments; ++i)
        {
            const float distance = position.getDistanceFrom(segments[i].pointAt(0.5f));
            if (distance < closest)
            {
                closest = distance;
                found = i;
            }
        }

        return found;
    }

    void setHovered(std::optional<size_t> handle)
    {
        setMouseCursor(handle ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        if (handle != hovered)
        {
            hovered = handle;
            repaint();
        }
    }

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
