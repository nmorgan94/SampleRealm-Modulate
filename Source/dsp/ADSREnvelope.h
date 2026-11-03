#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

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
    
    ADSREnvelope()
    {
        reset();
    }
    
    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }
    
    void setParameters(float attackTime, float decayTime, float sustainLevel, float releaseTime)
    {
        attack = juce::jmax(0.001f, attackTime);
        decay = juce::jmax(0.001f, decayTime);
        sustain = juce::jlimit(0.0f, 1.0f, sustainLevel);
        release = juce::jmax(0.001f, releaseTime);
    }
    
    void noteOn()
    {
        if (state == State::Idle)
        {
            currentLevel = 0.0f;
        }
        
        state = State::Attack;
        stateTime = 0.0;
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
                currentLevel = static_cast<float>(stateTime / attack);
                
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
                float decayProgress = static_cast<float>(stateTime / decay);
                currentLevel = 1.0f - (decayProgress * (1.0f - sustain));
                
                if (currentLevel <= sustain || stateTime >= decay)
                {
                    currentLevel = sustain;
                    state = State::Sustain;
                    stateTime = 0.0;
                }
                break;
            }
            
            case State::Sustain:
            {
                currentLevel = sustain;
                break;
            }
            
            case State::Release:
            {
                stateTime += timeIncrement;
                float releaseProgress = static_cast<float>(stateTime / release);
                currentLevel = releaseStartLevel * (1.0f - releaseProgress);
                
                if (currentLevel <= 0.0f || stateTime >= release)
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
            case State::Attack:  return juce::jlimit(0.0f, 1.0f, static_cast<float>(stateTime / attack));
            case State::Decay:   return juce::jlimit(0.0f, 1.0f, static_cast<float>(stateTime / decay));
            case State::Release: return juce::jlimit(0.0f, 1.0f, static_cast<float>(stateTime / release));
            default:             return 0.0f;
        }
    }

private:
    State state = State::Idle;
    double sampleRate = 44100.0;
    
    // ADSR parameters (in seconds)
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;
    
    // State tracking
    float currentLevel = 0.0f;
    double stateTime = 0.0;
    float releaseStartLevel = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSREnvelope)
};
