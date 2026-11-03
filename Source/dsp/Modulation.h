#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <span>

//==============================================================================
// A parameter's routing from an LFO or envelope. The range is in the parameter's 0..1 proportion
namespace Modulation
{
    inline constexpr int numSources = 4;

    // Saved by number in sessions
    enum class Source
    {
        None = 0,
        LFO,
        Envelope
    };

    inline Source sourceFromNumber(int number)
    {
        switch (number)
        {
            case 1:  return Source::LFO;
            case 2:  return Source::Envelope;
            default: return Source::None;
        }
    }

    struct Assignment
    {
        Source sourceType = Source::None;
        int sourceIndex = -1;
        float minValue = 0.0f;
        float maxValue = 1.0f;

        bool isAssigned() const { return sourceType != Source::None && sourceIndex >= 0 && sourceIndex < numSources; }
        bool isLFO() const { return sourceType == Source::LFO; }
        bool isEnvelope() const { return sourceType == Source::Envelope; }

        bool operator==(const Assignment& other) const
        {
            return sourceType == other.sourceType && sourceIndex == other.sourceIndex
                && juce::exactlyEqual(minValue, other.minValue) && juce::exactlyEqual(maxValue, other.maxValue);
        }
    };

    inline bool isSourceAssigned(std::span<const Assignment> assignments, Source sourceType, int sourceIndex)
    {
        return std::ranges::any_of(assignments, [&](const Assignment& assignment)
        {
            return assignment.isAssigned() && assignment.sourceType == sourceType && assignment.sourceIndex == sourceIndex;
        });
    }

    // The source's value (0..1) scaled into the range
    inline float modulatedProportion(const Assignment& assignment, float sourceValue)
    {
        return juce::jlimit(0.0f, 1.0f, assignment.minValue + sourceValue * (assignment.maxValue - assignment.minValue));
    }

    inline Assignment makeDefaultRange(Source sourceType, int sourceIndex, float base, float depth = 0.25f)
    {
        Assignment assignment;
        assignment.sourceType = sourceType;
        assignment.sourceIndex = sourceIndex;

        if (sourceType == Source::Envelope)
        {
            assignment.minValue = base;
            assignment.maxValue = juce::jmin(1.0f, base + depth);
        }
        else
        {
            assignment.minValue = juce::jlimit(0.0f, 1.0f, base - depth);
            assignment.maxValue = juce::jmin(1.0f, base + depth);
        }

        return assignment;
    }

    inline void applyDepthChange(Assignment& assignment, float base, float delta)
    {
        if (assignment.isEnvelope())
        {
            const float roomAbove = juce::jmax(0.0f, 1.0f - base);
            const float desiredDepth = (assignment.maxValue - assignment.minValue) + delta;
            const float depth = juce::jlimit(juce::jmin(0.05f, roomAbove), roomAbove, desiredDepth);

            assignment.minValue = base;
            assignment.maxValue = base + depth;
        }
        else
        {
            const float currentDepth = (assignment.maxValue - assignment.minValue) * 0.5f;
            const float desiredDepth = juce::jlimit(0.05f, 1.0f, currentDepth + delta);
            const float roomBelow = base, roomAbove = 1.0f - base;

            if (desiredDepth <= juce::jmin(roomBelow, roomAbove))
            {
                assignment.minValue = base - desiredDepth;
                assignment.maxValue = base + desiredDepth;
            }
            else
            {
                const float actualDepth = juce::jmin(desiredDepth, juce::jmax(roomBelow, roomAbove));
                assignment.minValue = juce::jmax(0.0f, base - actualDepth);
                assignment.maxValue = juce::jmin(1.0f, base + actualDepth);
            }
        }
    }
}
