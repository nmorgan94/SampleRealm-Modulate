#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Parameters.h"
#include "dsp/ModulateSynth.h"

class ModulationManager : private juce::ValueTree::Listener
{
public:
    using ModulationSource = Modulation::Source;
    using ModulationAssignment = Modulation::Assignment;

    ModulationManager() = default;

    // Each edit to the state, but not initialise
    std::function<void()> onEdit;

    ~ModulationManager() override
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
    
    // Apply an assignment struct directly (clears the routing when unassigned).
    void setAssignment(const juce::String& parameterID, const ModulationAssignment& assignment)
    {
        if (! assignment.isAssigned())
        {
            clearAssignment(parameterID);
            return;
        }

        if (! state.isValid())
            return;

        {
            // The audio thread sees the new assignment whole, not property by property
            const juce::ScopedValueSetter<bool> settingAssignment(isSettingAssignment, true);

            auto assignmentNode = findAssignment(parameterID);
            if (! assignmentNode.isValid())
            {
                assignmentNode = juce::ValueTree(IDs::assignment);
                assignmentNode.setProperty(IDs::parameterID, parameterID, undoManager);
                state.addChild(assignmentNode, -1, undoManager);
            }

            assignmentNode.setProperty(IDs::sourceType, static_cast<int>(assignment.sourceType), undoManager);
            assignmentNode.setProperty(IDs::sourceIndex, assignment.sourceIndex, undoManager);
            assignmentNode.setProperty(IDs::minValue, assignment.minValue, undoManager);
            assignmentNode.setProperty(IDs::maxValue, assignment.maxValue, undoManager);
        }

        rebuildSnapshot();
    }

    void clearAssignment(const juce::String& parameterID)
    {
        auto assignmentNode = findAssignment(parameterID);
        if (assignmentNode.isValid())
            state.removeChild(assignmentNode, undoManager);
    }
    
    ModulationAssignment getAssignment(const juce::String& parameterID) const
    {
        if (const auto index = Parameters::indexOf(parameterID))
            return getAssignments()[*index];

        return {};
    }

    // Every parameter's, in ParameterSpecs::all's order. Safe from the audio thread
    ModulateSynth::Assignments getAssignments() const
    {
        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        return snapshot;
    }

private:
    struct IDs
    {
        static inline const juce::Identifier assignment { "Assignment" };
        static inline const juce::Identifier parameterID { "parameterID" };
        static inline const juce::Identifier sourceType { "sourceType" };
        static inline const juce::Identifier sourceIndex { "sourceIndex" };
        static inline const juce::Identifier minValue { "minValue" };
        static inline const juce::Identifier maxValue { "maxValue" };
    };
    
    static ModulationAssignment toAssignment(const juce::ValueTree& assignmentNode)
    {
        return {
            Modulation::sourceFromNumber(assignmentNode.getProperty(IDs::sourceType, 0)),
            assignmentNode.getProperty(IDs::sourceIndex, -1),
            assignmentNode.getProperty(IDs::minValue, 0.0f),
            assignmentNode.getProperty(IDs::maxValue, 1.0f)
        };
    }
    
    juce::ValueTree findAssignment(const juce::String& parameterID) const
    {
        if (! state.isValid())
            return {};
        
        for (auto child : state)
        {
            if (child.hasType(IDs::assignment)
                && child[IDs::parameterID].toString() == parameterID)
                return child;
        }
        
        return {};
    }

    // On the message thread, after direct edits (through the ValueTree::Listener) and
    // preset loads (through initialise)
    void rebuildSnapshot()
    {
        if (isSettingAssignment)
            return;

        ModulateSynth::Assignments assignments {};

        if (state.isValid())
        {
            for (auto child : state)
            {
                if (! child.hasType(IDs::assignment))
                    continue;

                if (const auto index = Parameters::indexOf(child[IDs::parameterID].toString()))
                    assignments[*index] = toAssignment(child);
            }
        }

        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        snapshot = assignments;
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
    ModulateSynth::Assignments snapshot {};
    bool isSettingAssignment = false;
    mutable juce::SpinLock snapshotLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};
