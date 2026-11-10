#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** A rotary knob with no text box, which pops its value up while it's moved unless something else shows it. */
class RotarySlider : public juce::Slider
{
public:
    RotarySlider()
        : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox)
    {
    }

    void setShowsPopup(bool shouldShow)
    {
        showsPopup = shouldShow;
        updatePopupState();
    }

    /** A choice knob's names, the only text it takes as a typed value. */
    virtual juce::StringArray valueNames() const { return {}; }

protected:
    void parentHierarchyChanged() override { updatePopupState(); }

private:
    void updatePopupState()
    {
        setPopupDisplayEnabled(showsPopup, false, getTopLevelComponent());
    }

    bool showsPopup = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotarySlider)
};
