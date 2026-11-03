#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <optional>
#include "BaseCurveDisplay.h"
#include "../dsp/LfoCurve.h"

class CurveEditor : public BaseCurveDisplay,
                    public juce::Timer
{
public:
    using ControlPoint = LfoCurve::Point;

    CurveEditor() : tension(0.5f)
    {
        controlPoints = defaultControlPoints();
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    ~CurveEditor() override
    {
        stopTimer();
    }

    // The LFO's phase, 0..1, for the playhead
    void setPlayhead(const std::atomic<float>* phaseToShow)
    {
        playheadPhase = phaseToShow;
        updateTimerState();
    }
    
    void visibilityChanged() override
    {
        updateTimerState();
    }
    
    void updateTimerState()
    {
        if (isVisible() && playheadPhase != nullptr)
            startTimerHz(60);
        else
            stopTimer();
    }
    
    void timerCallback() override
    {
        if (playheadPhase != nullptr)
            repaint();
    }

protected:
    bool shouldDrawCenterLine() const override { return true; }
    
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
    {
        if (controlPoints.empty())
            return;
        
        // Generate smooth curve through control points
        const int resolution = 200;
        bool first = true;
        
        for (int i = 0; i <= resolution; ++i)
        {
            float x = i / (float)resolution;
            float value = getValueAt(x);
            
            float px = bounds.getX() + x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - value) * bounds.getHeight();
            
            if (first)
            {
                path.startNewSubPath(px, py);
                first = false;
            }
            else
            {
                path.lineTo(px, py);
            }
        }
    }
    
    void drawControlPoints(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        if (playheadPhase != nullptr)
        {
            float currentPhase = playheadPhase->load();
            if (currentPhase >= 0.0f && currentPhase <= 1.0f)
            {
                float playheadX = bounds.getX() + currentPhase * bounds.getWidth();
                g.setColour(juce::Colour(0xff00ff41).brighter(0.5f));
                g.drawLine(playheadX, bounds.getY(), playheadX, bounds.getBottom(), 2.0f);
            }
        }
        
        for (size_t i = 0; i < controlPoints.size(); ++i)
        {
            auto point = controlPoints[i];
            float px = bounds.getX() + point.x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - point.y) * bounds.getHeight();
            
            bool isHovered = (hoveredPointIndex.has_value() && i == hoveredPointIndex.value());
            bool isDragged = (draggedPointIndex.has_value() && i == draggedPointIndex.value());
            bool isHighlighted = isDragged || isHovered;
            
            drawPoint(g, px, py, isHighlighted);
        }
    }

public:
    void mouseMove(const juce::MouseEvent& event) override
    {
        hoveredPointIndex = findNearestPoint(event.position);
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredPointIndex.reset();
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isLeftButtonDown())
        {
            draggedPointIndex = findNearestPoint(event.position);
            
            // Double-click to add/remove points
            if (event.getNumberOfClicks() == 2)
            {
                if (draggedPointIndex.has_value())
                {
                    // Remove point (but keep first and last)
                    size_t pointIndex = draggedPointIndex.value();
                    if (pointIndex > 0 && pointIndex < controlPoints.size() - 1)
                    {
                        auto pointIterator = controlPoints.begin();
                        std::advance(pointIterator, pointIndex);
                        controlPoints.erase(pointIterator);
                        draggedPointIndex.reset();
                        notifyListeners();
                        repaint();
                    }
                }
                else
                {
                    // Add new point (if under limit)
                    if (controlPoints.size() < LfoCurve::maxPoints)
                    {
                        auto bounds = getLocalBounds().toFloat();
                        float x = juce::jlimit(0.0f, 1.0f, (event.position.x - bounds.getX()) / bounds.getWidth());
                        float y = juce::jlimit(0.0f, 1.0f, 1.0f - (event.position.y - bounds.getY()) / bounds.getHeight());
                        
                        // Find where to insert to keep points sorted by X position
                        auto insertPosition = std::lower_bound(controlPoints.begin(), controlPoints.end(), x,
                            [](const ControlPoint& p, float val) { return p.x < val; });
                        controlPoints.insert(insertPosition, ControlPoint(x, y));
                        notifyListeners();
                        repaint();
                    }
                }
            }
        }
    }
    
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (draggedPointIndex.has_value() && draggedPointIndex.value() < controlPoints.size())
        {
            auto bounds = getLocalBounds().toFloat();
            float x = (event.position.x - bounds.getX()) / bounds.getWidth();
            float y = 1.0f - (event.position.y - bounds.getY()) / bounds.getHeight();
            
            // Clamp Y value
            y = juce::jlimit(0.0f, 1.0f, y);
            
            // Constrain X position based on point type
            if (draggedPointIndex.value() == 0)
            {
                // First point stays at x=0
                x = 0.0f;
                // Sync last point's Y value for seamless loop
                controlPoints.back().y = y;
            }
            else if (draggedPointIndex.value() == controlPoints.size() - 1)
            {
                // Last point stays at x=1
                x = 1.0f;
                // Sync first point's Y value for seamless loop
                controlPoints.front().y = y;
            }
            else
            {
                // Middle points: constrain between neighbors with small margin
                const float margin = 0.01f; // Minimum distance between points
                float minX = controlPoints[draggedPointIndex.value() - 1].x + margin;
                float maxX = controlPoints[draggedPointIndex.value() + 1].x - margin;
                x = juce::jlimit(minX, maxX, x);
            }
            
            controlPoints[draggedPointIndex.value()].x = x;
            controlPoints[draggedPointIndex.value()].y = y;
            
            notifyListeners();
            repaint();
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        draggedPointIndex.reset();
        repaint();
    }
    
    // Get interpolated value at position x (0.0 to 1.0) using Catmull-Rom spline
    float getValueAt(float x) const
    {
        return LfoCurve::evaluate(controlPoints, tension, x);
    }

    static std::vector<ControlPoint> defaultControlPoints()
    {
        return { LfoCurve::defaultPoints.begin(), LfoCurve::defaultPoints.end() };
    }

    const std::vector<ControlPoint>& getControlPoints() const { return controlPoints; }
    
    void setControlPoints(const std::vector<ControlPoint>& points)
    {
        controlPoints = points;
        repaint();
    }
    
    void setTension(float newTension)
    {
        tension = juce::jlimit(0.0f, 1.0f, newTension);
        repaint();
    }
    
    // Listener interface for curve changes
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void curveChanged(CurveEditor* editor) = 0;
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }

private:
    std::optional<size_t> findNearestPoint(juce::Point<float> pos) const
    {
        auto bounds = getLocalBounds().toFloat();
        const float threshold = 10.0f;
        std::optional<size_t> nearest;
        float minDist = threshold;
        
        for (size_t i = 0; i < controlPoints.size(); ++i)
        {
            float px = bounds.getX() + controlPoints[i].x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - controlPoints[i].y) * bounds.getHeight();
            
            float dist = pos.getDistanceFrom(juce::Point<float>(px, py));
            if (dist < minDist)
            {
                minDist = dist;
                nearest = i;
            }
        }
        
        return nearest;
    }
    
    void notifyListeners()
    {
        listeners.call([this](Listener& l) { l.curveChanged(this); });
    }
    
    std::vector<ControlPoint> controlPoints;
    std::optional<size_t> hoveredPointIndex;
    std::optional<size_t> draggedPointIndex;
    juce::ListenerList<Listener> listeners;
    const std::atomic<float>* playheadPhase = nullptr;
    float tension;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor)
};
