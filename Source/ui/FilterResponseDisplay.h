#pragma once

#include "BaseCurveDisplay.h"
#include "CustomLookAndFeel.h"
#include "../dsp/Filter.h"
#include <cmath>

class FilterResponseDisplay : public BaseCurveDisplay
{
public:
    FilterResponseDisplay() : BaseCurveDisplay(false)
    {
        responseFilter.prepare(sampleRate);
    }
    
    void setFilterParameters(FilterMode mode, FilterSlope slope, float cutoffHz, float resonance, double sr)
    {
        if (sr <= 0.0)
            return;

        if (! juce::approximatelyEqual(sr, sampleRate))
        {
            sampleRate = sr;
            responseFilter.prepare(sampleRate);
        }

        responseFilter.setParameters(mode, slope, cutoffHz, resonance);
        repaint();
    }
    
    void setEnabled(bool shouldBeEnabled)
    {
        isEnabled = shouldBeEnabled;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        juce::Path curvePath;
        generateCurvePath(curvePath, bounds);
        
        juce::Path filledPath = curvePath;
        filledPath.lineTo(bounds.getRight(), bounds.getBottom());
        filledPath.lineTo(bounds.getX(), bounds.getBottom());
        filledPath.closeSubPath();
        
        auto baseColor = isEnabled ? juce::Colour(0xff00ff41) : juce::Colour(0xff666666);
        
        g.setColour(baseColor.withAlpha(0.2f));
        g.fillPath(filledPath);
        
        g.setColour(baseColor);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));
        
        drawLabels(g, bounds);
    }

protected:
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
    {
        if (sampleRate <= 0.0f)
            return;
        
        constexpr int numPoints = 200;
        
        for (int i = 0; i < numPoints; ++i)
        {
            float t = i / float(numPoints - 1);
            float freq = 20.0f * std::pow(1000.0f, t);
            float magnitude = calculateMagnitudeResponse(freq);
            float magnitudeDB = 20.0f * std::log10(std::max(magnitude, 0.00001f));
            float normalizedMag = juce::jlimit(0.0f, 1.0f, juce::jmap(magnitudeDB, -60.0f, 20.0f, 1.0f, 0.0f));
            
            float x = bounds.getX() + t * bounds.getWidth();
            float y = bounds.getY() + normalizedMag * bounds.getHeight();
            
            i == 0 ? path.startNewSubPath(x, y) : path.lineTo(x, y);
        }
    }
    
    void drawLabels(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        auto labelColor = isEnabled ? juce::Colour(0xff666666) : juce::Colour(0xff3a3a3a);
        g.setColour(labelColor);
        g.setFont(CustomLookAndFeel::orbitron(CustomLookAndFeel::regularWeight).withHeight(10.0f));
        
        const float labelY = bounds.getBottom() - 12.0f;
        g.drawText("20", juce::Rectangle<float>(bounds.getX(), labelY, 30.0f, 12.0f), juce::Justification::left, false);
        g.drawText("1k", juce::Rectangle<float>(bounds.getCentreX() - 10.0f, labelY, 20.0f, 12.0f), juce::Justification::centred, false);
        g.drawText("20k", juce::Rectangle<float>(bounds.getRight() - 30.0f, labelY, 30.0f, 12.0f), juce::Justification::right, false);
    }

private:
    Filter responseFilter;
    double sampleRate = 44100.0;
    bool isEnabled = true;

    float calculateMagnitudeResponse(float frequency) const
    {
        return frequency > 0.0f ? responseFilter.getMagnitudeAt(frequency) : 1.0f;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterResponseDisplay)
};
