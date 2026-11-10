#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "RotarySlider.h"

//==============================================================================
/** A knob with its name above, which shows the value while it's being moved. Click the name to type a value. */
template <typename SliderType = RotarySlider>
class LabeledSlider : public juce::Component,
                      private juce::Slider::Listener,
                      private juce::Timer
{
    static_assert (std::is_base_of_v<RotarySlider, SliderType>);

public:
    template <typename... Args>
    explicit LabeledSlider (Args&&... sliderArgs)
        : slider (std::forward<Args> (sliderArgs)...)
    {
        slider.setShowsPopup (false);
        slider.addListener (this);

        label.setJustificationType (juce::Justification::centred);
        label.setEditable (true);
        label.onEditorShow = [this]
        {
            stopTimer(); // showing the name would close the editor
            if (auto* editor = label.getCurrentTextEditor())
            {
                editor->setText (valueText(), false);
                editor->selectAll();
            }
        };
        label.onEditorHide = [this] { startTimer (nameDelayMs); };
        // As Slider's text box, but snapped as setValue does, and ignoring anything but a number or one of the knob's names
        label.onTextChange = [this]
        {
            const auto typed = label.getText().trim();
            const auto names = slider.valueNames();

            // As JavaScript's parseFloat, it must start with a number. A name is passed on as the knob spells it
            const auto text = names.isEmpty() ? (typed.initialSectionContainingOnly ("+-.0123456789").containsAnyOf ("0123456789") ? typed : juce::String())
                                              : names[names.indexOf (typed, true)];

            if (text.isNotEmpty())
            {
                const auto typedValue = slider.getNormalisableRange().snapToLegalValue (
                    slider.snapValue (slider.getValueFromText (text), juce::Slider::notDragging));

                if (! juce::approximatelyEqual (typedValue, slider.getValue()))
                {
                    juce::Slider::ScopedDragNotification drag (slider);
                    slider.setValue (typedValue, juce::sendNotificationSync);
                }
            }

            showValue();
        };

        addAndMakeVisible (slider);
        addAndMakeVisible (label);
    }

    /** Also the slider's title, which screen readers read. */
    void setLabelText (const juce::String& newText)
    {
        setName (newText);
        slider.setTitle (newText);
        showName();
    }

    void setLabelColour (juce::Colour colour) { label.setColour (juce::Label::textColourId, colour); }

    SliderType& getSlider() noexcept { return slider; }

    static constexpr int labelHeight = 15;

    /** The label along the top, and the knob square and centred under it. */
    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (labelHeight));
        const auto size = std::max (0, std::min (bounds.getWidth(), bounds.getHeight()));
        slider.setBounds (bounds.withSizeKeepingCentre (size, size));
    }

private:
    static constexpr int nameDelayMs = 600;

    SliderType slider;
    juce::Label label;
    bool dragging = false;

    void showName()             { label.setText (getName(), juce::dontSendNotification); }
    juce::String valueText()    { return slider.getTextFromValue (slider.getValue()); }
    void showValue()            { label.setText (valueText(), juce::dontSendNotification); }

    // The mouse wheel, double-click reset and typed values are drags too
    void sliderDragStarted (juce::Slider*) override     { stopTimer(); dragging = true; showValue(); }
    void sliderValueChanged (juce::Slider*) override    { if (dragging || isTimerRunning()) showValue(); }
    void sliderDragEnded (juce::Slider*) override       { dragging = false; startTimer (nameDelayMs); }

    void timerCallback() override
    {
        stopTimer();
        showName();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LabeledSlider)
};
