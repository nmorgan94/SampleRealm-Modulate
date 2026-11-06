#pragma once

#include "ModalOverlay.h"

//==============================================================================
/**
 * An in-editor dialog that dims its parent.
 */
class PromptOverlay : public ModalOverlay
{
public:
    PromptOverlay()
    {
        messageLabel.setFont (juce::FontOptions (12.0f));
        messageLabel.setJustificationType (juce::Justification::topLeft);
        messageLabel.setBorderSize ({});

        textEditor.onReturnKey = [this] { accept(); };
        textEditor.onEscapeKey = [this] { dismiss(); };

        okButton.onClick     = [this] { accept(); };
        cancelButton.onClick = [this] { dismiss(); };

        addAndMakeVisible (okButton);

        // Shown per prompt
        for (auto* c : std::initializer_list<juce::Component*> { &messageLabel, &textEditor, &cancelButton })
            addChildComponent (c);
    }

    /** Font for the text field; labels and buttons use the LookAndFeel's. */
    void setFont (const juce::Font& font)   { textEditor.setFont (font); }

    void askForText (const juce::String& title, const juce::String& initialText, const juce::String& okText,
                     std::function<void (const juce::String&)> onOk)
    {
        show (title, {}, okText, true, true, std::move (onOk));
        textEditor.setText (initialText, false);
        textEditor.selectAll();
        textEditor.grabKeyboardFocus();
    }

    void askToConfirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                       std::function<void()> onOk)
    {
        show (title, message, okText, false, true, [confirmed = std::move (onOk)] (const juce::String&) { confirmed(); });
    }

    void showMessage (const juce::String& title, const juce::String& message)
    {
        show (title, message, "OK", false, false, nullptr);
    }

    void dismiss() override
    {
        callback = nullptr;
        ModalOverlay::dismiss();
    }

    //==============================================================================
    void resized() override
    {
        auto area = getPanelBounds().reduced (20, 16);

        titleLabel.setBounds (area.removeFromTop (titleHeight));
        area.removeFromTop (8);

        auto buttons = area.removeFromBottom (28);
        okButton.setBounds (buttons.removeFromRight (90));
        buttons.removeFromRight (8);
        cancelButton.setBounds (buttons.removeFromRight (90));
        area.removeFromBottom (12);

        textEditor.setBounds (area.withHeight (28));
        messageLabel.setBounds (area);
    }

private:
    juce::Label messageLabel;
    juce::TextEditor textEditor;
    juce::TextButton okButton, cancelButton { "Cancel" };
    std::function<void (const juce::String&)> callback;
    int extraHeight = 0;

    juce::Rectangle<int> getPanelBounds() const override
    {
        return getLocalBounds().withSizeKeepingCentre (380, juce::jmin (160 + extraHeight, getHeight()));
    }

    void panelKeyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::returnKey)
            accept();
    }

    void show (const juce::String& title, const juce::String& message, const juce::String& okText,
               bool wantsText, bool canCancel, std::function<void (const juce::String&)> onOk)
    {
        callback = std::move (onOk);
        titleLabel.setText (title, juce::dontSendNotification);
        messageLabel.setText (message, juce::dontSendNotification);
        okButton.setButtonText (okText);

        messageLabel.setVisible (! wantsText);
        textEditor.setVisible (wantsText);
        cancelButton.setVisible (canCancel);

        // Lay out at the base size, then grow by what the message overflows
        extraHeight = 0;
        parentSizeChanged();
        resized();
        extraHeight = juce::jmax (0, wrappedHeight() - messageLabel.getHeight());
        open();
    }

    // As the label wraps it, with no line limit
    int wrappedHeight()
    {
        juce::GlyphArrangement glyphs;
        glyphs.addFittedText (getLookAndFeel().getLabelFont (messageLabel), messageLabel.getText(), 0.0f, 0.0f,
                              messageLabel.getLocalBounds().toFloat().getWidth(),
                              std::numeric_limits<float>::max(), messageLabel.getJustificationType(),
                              std::numeric_limits<int>::max(), messageLabel.getMinimumHorizontalScale());

        return juce::roundToInt (glyphs.getBoundingBox (0, -1, true).getBottom());
    }

    /** Closes first, so the callback can open another prompt. */
    void accept()
    {
        auto onOk = std::move (callback);
        const auto text = textEditor.getText();
        dismiss();

        if (onOk != nullptr)
            onOk (text);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PromptOverlay)
};
