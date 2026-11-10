#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "EnvelopeCurve.h"

class ADSREnvelope
{
public:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };
    
    // Times in seconds
    struct Parameters
    {
        float attack = 0.01f, decay = 0.1f, sustain = 0.7f, release = 0.3f;
        float attackCurve = 0.0f, decayCurve = 0.0f, releaseCurve = 0.0f;

        bool operator==(const Parameters& other) const { return values() == other.values(); }

        std::array<float, 7> values() const { return { attack, decay, sustain, release, attackCurve, decayCurve, releaseCurve }; }
    };

    ADSREnvelope()
    {
        reset();
    }
    
    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }
    
    void setParameters(Parameters newParameters)
    {
        newParameters.attack = juce::jmax(0.001f, newParameters.attack);
        newParameters.decay = juce::jmax(0.001f, newParameters.decay);
        newParameters.sustain = juce::jlimit(0.0f, 1.0f, newParameters.sustain);
        newParameters.release = juce::jmax(0.001f, newParameters.release);

        if (newParameters == parameters)
            return;

        parameters = newParameters;
        continueFromCurrentLevel();
    }
    
    void noteOn()
    {
        if (state == State::Idle)
        {
            currentLevel = 0.0f;
        }
        
        state = State::Attack;
        continueFromCurrentLevel();
    }
    
    void noteOff()
    {
        if (state != State::Idle)
        {
            state = State::Release;
            releaseStartLevel = currentLevel;
            stateTime = 0.0;
        }
    }
    
    void reset()
    {
        state = State::Idle;
        currentLevel = 0.0f;
        stateTime = 0.0;
        releaseStartLevel = 0.0f;
    }
    
    float getNextSample()
    {
        if (state == State::Idle)
            return 0.0f;
        
        const double timeIncrement = 1.0 / sampleRate;
        
        switch (state)
        {
            case State::Attack:
            {
                stateTime += timeIncrement;
                const float attackProgress = static_cast<float>(stateTime / parameters.attack);
                currentLevel = EnvelopeCurve::shapeProgress(attackProgress, parameters.attackCurve);
                
                if (currentLevel >= 1.0f)
                {
                    currentLevel = 1.0f;
                    state = State::Decay;
                    stateTime = 0.0;
                }
                break;
            }
            
            case State::Decay:
            {
                stateTime += timeIncrement;
                float decayProgress = static_cast<float>(stateTime / parameters.decay);
                currentLevel = 1.0f - (EnvelopeCurve::shapeProgress(decayProgress, parameters.decayCurve) * (1.0f - parameters.sustain));
                
                if (currentLevel <= parameters.sustain || stateTime >= parameters.decay)
                {
                    currentLevel = parameters.sustain;
                    state = State::Sustain;
                    stateTime = 0.0;
                }
                break;
            }
            
            case State::Sustain:
            {
                const auto maximumStep = static_cast<float>(timeIncrement / sustainGlideTime);
                currentLevel += juce::jlimit(-maximumStep, maximumStep, parameters.sustain - currentLevel);
                break;
            }
            
            case State::Release:
            {
                stateTime += timeIncrement;
                float releaseProgress = static_cast<float>(stateTime / parameters.release);
                currentLevel = releaseStartLevel * (1.0f - EnvelopeCurve::shapeProgress(releaseProgress, parameters.releaseCurve));
                
                if (currentLevel <= 0.0f || stateTime >= parameters.release)
                {
                    currentLevel = 0.0f;
                    state = State::Idle;
                    stateTime = 0.0;
                }
                break;
            }
            
            case State::Idle:
            default:
                currentLevel = 0.0f;
                break;
        }
        
        return juce::jlimit(0.0f, 1.0f, currentLevel);
    }
    
    bool isActive() const
    {
        return state != State::Idle;
    }
    
    State getState() const
    {
        return state;
    }
    
    float getCurrentLevel() const
    {
        return currentLevel;
    }

    float getStageProgress() const
    {
        switch (state)
        {
            case State::Attack:  return juce::jlimit(0.0f, 1.0f, static_cast<float>(stateTime / parameters.attack));
            case State::Decay:   return juce::jlimit(0.0f, 1.0f, static_cast<float>(stateTime / parameters.decay));
            case State::Release: return juce::jlimit(0.0f, 1.0f, static_cast<float>(stateTime / parameters.release));
            default:             return 0.0f;
        }
    }

private:
    // Picks up the stage from the current level
    void continueFromCurrentLevel()
    {
        switch (state)
        {
            case State::Attack:
                stateTime = parameters.attack * EnvelopeCurve::progressAt(currentLevel, parameters.attackCurve);
                break;

            case State::Decay:
                if (currentLevel <= parameters.sustain)
                {
                    state = State::Sustain;
                    stateTime = 0.0;
                }
                else
                {
                    stateTime = parameters.decay * EnvelopeCurve::progressAt((1.0f - currentLevel) / (1.0f - parameters.sustain), parameters.decayCurve);
                }
                break;

            case State::Release:
                if (releaseStartLevel > 0.0f)
                    stateTime = parameters.release * EnvelopeCurve::progressAt(1.0f - currentLevel / releaseStartLevel, parameters.releaseCurve);
                break;

            case State::Idle:
            case State::Sustain:
            default:
                break;
        }
    }

    State state = State::Idle;
    double sampleRate = 44100.0;
    
    Parameters parameters;

    // Seconds for the sustain level to move all the way
    static constexpr double sustainGlideTime = 0.01;
    
    // State tracking
    float currentLevel = 0.0f;
    double stateTime = 0.0;
    float releaseStartLevel = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSREnvelope)
};
