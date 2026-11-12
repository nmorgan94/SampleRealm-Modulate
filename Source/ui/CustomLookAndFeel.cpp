#include "CustomLookAndFeel.h"
#include <BinaryData.h>
#include <map>

namespace
{
    // FontOptions can't take both a typeface and variable settings, so each weight is its own clone
    juce::Typeface::Ptr orbitronTypeface (float weight)
    {
        static const auto variable = juce::Typeface::createSystemTypefaceFor (
            BinaryData::OrbitronVariable_ttf, BinaryData::OrbitronVariable_ttfSize);

        static juce::CriticalSection lock;
        static std::map<float, juce::Typeface::Ptr> byWeight;

        const juce::ScopedLock scope (lock);
        auto& typeface = byWeight[weight];

        if (typeface == nullptr)
        {
            const juce::FontVariableSetting setting { "wght", weight };
            typeface = variable->cloneWithVariableSettings ({ &setting, 1 });
        }

        return typeface;
    }
}

//==============================================================================
CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ff41));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xff00ff41));

    setColour(juce::BubbleComponent::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::BubbleComponent::outlineColourId, juce::Colour(0xff00ff41));
    setColour(juce::TooltipWindow::textColourId, juce::Colour(0xff00ff41));

    setColour(ModalOverlay::dimColourId, juce::Colours::black.withAlpha(0.6f));
    setColour(ModalOverlay::panelColourId, juce::Colour(0xff1a1a1a));
    setColour(ModalOverlay::outlineColourId, juce::Colour(0xff00ff41));

    setColour(HeaderBar::backgroundColourId, juce::Colours::transparentBlack);
    setColour(HeaderBar::titleColourId, juce::Colour(0xff00ff00));

    setColour(LevelMeter::backgroundColourId, juce::Colours::black);
    setColour(LevelMeter::outlineColourId, juce::Colour(0xff303030));

    setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    setColour(juce::TextEditor::textColourId, juce::Colours::white);
    setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff00ff41));
    setColour(juce::TextEditor::highlightColourId, juce::Colour(0xff00ff41).withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour(juce::CaretComponent::caretColourId, juce::Colour(0xff00ff41));

    setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff3a3a3a));

    // TextButton's background and outline, and --color-widget and --color-line in webui/src/styles.css
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff303030));
}

juce::FontOptions CustomLookAndFeel::orbitron (float weight)
{
    return juce::FontOptions (orbitronTypeface (weight));
}
