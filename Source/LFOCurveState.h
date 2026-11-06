#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/ModulateSynth.h"

class LFOCurveState : private juce::ValueTree::Listener
{
public:
    LFOCurveState() = default;

    // Each edit to the state, but not initialise
    std::function<void()> onEdit;

    ~LFOCurveState() override
    {
        if (state.isValid())
            state.removeListener(this);
    }
    
    void initialise(juce::ValueTree treeState, juce::UndoManager* treeUndoManager)
    {
        if (state.isValid())
            state.removeListener(this);

        state = treeState;
        undoManager = treeUndoManager;

        if (state.isValid())
            state.addListener(this);

        rebuildSnapshot();
    }
    
    std::vector<LfoCurve::Point> getCurvePoints(size_t lfoIndex) const
    {
        std::vector<LfoCurve::Point> points;
        auto curveNode = findCurve(static_cast<int>(lfoIndex));
        
        if (! curveNode.isValid())
            return points;
        
        points.reserve(static_cast<size_t>(curveNode.getNumChildren()));
        
        for (auto pointNode : curveNode)
        {
            // All the DSP keeps
            if (points.size() == LfoCurve::maxPoints)
                break;

            if (pointNode.hasType(IDs::point))
            {
                points.emplace_back(
                    static_cast<float>(pointNode.getProperty(IDs::x, 0.0f)),
                    static_cast<float>(pointNode.getProperty(IDs::y, 0.5f))
                );
            }
        }
        
        return points;
    }
    
    void setCurvePoints(size_t lfoIndex, const std::vector<LfoCurve::Point>& points)
    {
        if (! state.isValid())
            return;
        
        {
            // The audio thread sees the new curve whole, not point by point
            const juce::ScopedValueSetter<bool> settingPoints(isSettingPoints, true);

            auto curveNode = findOrCreateCurve(static_cast<int>(lfoIndex));

            curveNode.removeAllChildren(undoManager);

            for (const auto& point : std::span(points).first(std::min(points.size(), LfoCurve::maxPoints)))
            {
                juce::ValueTree pointNode(IDs::point);
                pointNode.setProperty(IDs::x, point.x, undoManager);
                pointNode.setProperty(IDs::y, point.y, undoManager);
                curveNode.addChild(pointNode, -1, undoManager);
            }
        }

        rebuildSnapshot();
    }

    // Safe from the audio thread
    ModulateSynth::Curves getCurves() const
    {
        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        return snapshot;
    }

private:
    struct IDs
    {
        static inline const juce::Identifier curve { "LFOCurve" };
        static inline const juce::Identifier point { "Point" };
        static inline const juce::Identifier index { "index" };
        static inline const juce::Identifier x { "x" };
        static inline const juce::Identifier y { "y" };
    };
    
    juce::ValueTree findCurve(int lfoIndex) const
    {
        if (! state.isValid())
            return {};
        
        for (auto child : state)
        {
            if (child.hasType(IDs::curve)
                && static_cast<int>(child.getProperty(IDs::index, -1)) == lfoIndex)
                return child;
        }
        
        return {};
    }
    
    juce::ValueTree findOrCreateCurve(int lfoIndex)
    {
        auto curveNode = findCurve(lfoIndex);
        if (curveNode.isValid())
            return curveNode;
        
        curveNode = juce::ValueTree(IDs::curve);
        curveNode.setProperty(IDs::index, lfoIndex, undoManager);
        state.addChild(curveNode, -1, undoManager);
        return curveNode;
    }

    void rebuildSnapshot()
    {
        if (isSettingPoints)
            return;

        ModulateSynth::Curves curves {};

        for (size_t i = 0; i < curves.size(); ++i)
        {
            const auto points = getCurvePoints(i);
            auto& curve = curves[i];
            curve.count = points.size();
            std::ranges::copy(points, curve.points.begin());
        }

        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        snapshot = curves;
    }

    void edited()
    {
        rebuildSnapshot();

        if (onEdit)
            onEdit();
    }

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override { edited(); }
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override { edited(); }
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override { edited(); }
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override { edited(); }
    void valueTreeParentChanged(juce::ValueTree&) override { edited(); }
    
    juce::ValueTree state;
    juce::UndoManager* undoManager = nullptr;
    ModulateSynth::Curves snapshot {};
    mutable juce::SpinLock snapshotLock;
    bool isSettingPoints = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOCurveState)
};
