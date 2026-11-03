#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
enum class WaveformType
{
    Sine = 0,
    Saw,
    Square,
    Triangle,
    Pulse25,
    Pulse10,
    FormantSine,
    FormantVowel,
    BitcrushedRamp,
    FmOperator,
    FeedbackFm,
    WavefoldedSine,
    FractalPulse,
    DoubleSaw,
    ResonantSaw,
    PrimeCluster,
    OddHarmonicCluster,
    LogSquare,
    AsymmetricRectifiedSine,
    InterleavedSawPulse,
    Trapezoid
};

//==============================================================================
class WavetableGenerator
{
public:
    /**
     * @param buffer The AudioBuffer to fill with wavetable data (should be pre-sized)
     * @param type The waveform type to generate
     */
    static void generateWavetable(juce::AudioBuffer<float>& buffer, WaveformType type)
    {
        jassert(buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0);
        
        const int wavetableSize = buffer.getNumSamples();
        auto* samples = buffer.getWritePointer(0);
        
        switch (type)
        {
            case WaveformType::Sine:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    auto phase = juce::MathConstants<float>::twoPi * i / wavetableSize;
                    samples[i] = std::sin(phase);
                }
                break;
            }
            
            case WaveformType::Saw:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    samples[i] = 2.0f * i / wavetableSize - 1.0f;
                }
                break;
            }
            
            case WaveformType::Square:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    samples[i] = (i < wavetableSize / 2) ? 1.0f : -1.0f;
                }
                break;
            }
            
            case WaveformType::Triangle:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    if (i < wavetableSize / 2)
                        samples[i] = -1.0f + 4.0f * i / wavetableSize;
                    else
                        samples[i] = 3.0f - 4.0f * i / wavetableSize;
                }
                break;
            }
            
            case WaveformType::Pulse25:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    samples[i] = (i < wavetableSize / 4) ? 1.0f : -1.0f;
                }
                break;
            }
            
            case WaveformType::Pulse10:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    samples[i] = (i < wavetableSize / 10) ? 1.0f : -1.0f;
                }
                break;
            }
            
            case WaveformType::FormantSine:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = juce::MathConstants<float>::twoPi * i / wavetableSize;
                    float baseSine = std::sin(phase);
                    samples[i] = std::pow(std::abs(baseSine), 0.5f) * (baseSine > 0 ? 1 : -1);
                }
                break;
            }

            case WaveformType::FormantVowel:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float fundamental = std::sin(juce::MathConstants<float>::twoPi * phase);
                    float resonance = std::sin(juce::MathConstants<float>::twoPi * phase * 5.0f) * 0.4f;
                    samples[i] = juce::jlimit(-1.0f, 1.0f, fundamental + resonance);
                }
                break;
            }
            case WaveformType::BitcrushedRamp:
            {
                const int steps = 8; // Quantization steps
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float continuousSaw = 1.0f - (2.0f * phase);
                    samples[i] = std::round(continuousSaw * steps) / steps;
                }
                break;
            }
            case WaveformType::FmOperator:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float modulator = std::sin(juce::MathConstants<float>::twoPi * phase * 3.0f) * 1.5f; // 3:1 Ratio, Index 1.5
                    float carrierPhase = (juce::MathConstants<float>::twoPi * phase) + modulator;
                    samples[i] = std::sin(carrierPhase);
                }
                break;
            }
            case WaveformType::FeedbackFm:
            {
                float lastSample = 0.0f;
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float currentPhase = (juce::MathConstants<float>::twoPi * phase) + (lastSample * 0.8f);
                    lastSample = std::sin(currentPhase);
                    samples[i] = lastSample;
                }
                break;
            }
            case WaveformType::WavefoldedSine:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float drivenSine = std::sin(juce::MathConstants<float>::twoPi * phase) * 3.0f; // Overdrive
                    if (drivenSine > 1.0f)  drivenSine = 2.0f - drivenSine;
                    if (drivenSine < -1.0f) drivenSine = -2.0f - drivenSine;
                    samples[i] = drivenSine;
                }
                break;
            }
            case WaveformType::FractalPulse:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    int quadrant = (i * 4) / wavetableSize;
                    switch (quadrant)
                    {
                        case 0: samples[i] = 1.0f; break;
                        case 1: samples[i] = 0.25f; break;
                        case 2: samples[i] = -1.0f; break;
                        case 3: samples[i] = -0.25f; break;
                    }
                }
                break;
            }
            case WaveformType::DoubleSaw:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float saw1 = 1.0f - (2.0f * phase);
                    float phase2 = std::fmod(phase * 2.0f, 1.0f);
                    float saw2 = 1.0f - (2.0f * phase2);
                    samples[i] = (saw1 * 0.6f) - (saw2 * 0.4f);
                }
                break;
            }
            case WaveformType::ResonantSaw:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float saw = 1.0f - (2.0f * phase);
                    float ringing = std::sin(juce::MathConstants<float>::twoPi * phase * 12.0f) * std::exp(-phase * 4.0f);
                    samples[i] = juce::jlimit(-1.0f, 1.0f, saw + (ringing * 0.5f));
                }
                break;
            }
            case WaveformType::PrimeCluster:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float rad = juce::MathConstants<float>::twoPi * phase;
                    samples[i] = (std::sin(rad * 2.0f) * 0.6f) + (std::sin(rad * 5.0f) * 0.3f) + (std::sin(rad * 11.0f) * 0.1f);
                }
                break;
            }
            case WaveformType::OddHarmonicCluster:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float rad = juce::MathConstants<float>::twoPi * phase;
                    samples[i] = (std::sin(rad) * 1.0f) + (std::sin(rad * 3.0f) * 0.5f) + (std::sin(rad * 5.0f) * 0.25f) + (std::sin(rad * 7.0f) * 0.125f);
                    samples[i] *= 0.533f;
                }
                break;
            }
            case WaveformType::LogSquare:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float rawSine = std::sin(juce::MathConstants<float>::twoPi * phase);
                    samples[i] = (rawSine >= 0.0f) ? std::log1p(rawSine * 9.0f) / std::log1p(9.0f) : -std::log1p(-rawSine * 9.0f) / std::log1p(9.0f);
                }
                break;
            }
            case WaveformType::AsymmetricRectifiedSine:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float sinVal = std::sin(juce::MathConstants<float>::twoPi * phase);
                    samples[i] = (sinVal >= 0.0f) ? sinVal : std::abs(sinVal) * 0.5f - 0.5f;
                }
                break;
            }
            case WaveformType::InterleavedSawPulse:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    if (phase < 0.5f)
                    {
                        samples[i] = 1.0f - (4.0f * phase);
                    }
                    else
                    {
                        samples[i] = (phase < 0.75f) ? 1.0f : -1.0f;
                    }
                }
                break;
            }
            case WaveformType::Trapezoid:
            {
                for (int i = 0; i < wavetableSize; ++i)
                {
                    float phase = (float)i / wavetableSize;
                    float tri = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                    samples[i] = juce::jlimit(-0.7f, 0.7f, tri) * (1.0f / 0.7f);
                }
                break;
            }
        }
    }
    
    static constexpr int getWavetableSize() { return 2048; }   

private:
    WavetableGenerator() = delete;
    ~WavetableGenerator() = delete;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableGenerator)
};
