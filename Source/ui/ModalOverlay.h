#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * A panel over its parent that dims it and takes every key and click until dismissed.
 * Mirrored by webui/src/Modal.tsx.
 */
class ModalOverlay : public juce::Component
{
public:
    enum ColourIds
    {
        dimColourId = 0x5d10400,
        panelColourId,
        outlineColourId
    };

    ModalOverlay()
    {
        titleLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        titleLabel.setBorderSize ({});
        addAndMakeVisible (titleLabel);
        setWantsKeyboardFocus (true);
    }

    virtual void dismiss()   { setVisible (false); }

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.fillAll (findColour (dimColourId));

        const auto panel = getPanelBounds().toFloat();
        g.setColour (findColour (panelColourId));
        g.fillRoundedRectangle (panel, 8.0f);
        g.setColour (findColour (outlineColourId));
        g.drawRoundedRectangle (panel.reduced (0.5f), 8.0f, 1.0f);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
            dismiss();
        else
            panelKeyPressed (key);

        return true; // nothing behind the overlay gets keys
    }

    void mouseDown (const juce::MouseEvent&) override {} // swallows clicks outside the panel

    void parentSizeChanged() override
    {
        if (auto* parent = getParentComponent())
            setBounds (parent->getLocalBounds());
    }

protected:
    static constexpr int titleHeight = 24;

    /** Laid out by each overlay. */
    juce::Label titleLabel;

    void open()
    {
        parentSizeChanged();

        // The panel may depend on what's shown
        resized();
        repaint();
        setVisible (true);
        toFront (true);
    }

    virtual juce::Rectangle<int> getPanelBounds() const = 0;

    /** Any key but Escape. */
    virtual void panelKeyPressed (const juce::KeyPress&) {}
};
