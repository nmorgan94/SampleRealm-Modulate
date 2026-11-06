#pragma once

#include "ModalOverlay.h"

// Mirrored by webui/src/manual.ts
namespace Manual
{
    struct Block
    {
        enum class Kind { heading, subheading, bullet, paragraph };

        Kind kind;
        juce::String text;

        bool operator== (const Block&) const = default;
    };

    /** "# ", "## " and "- " start a heading, a subheading and a bullet. Other lines are text, joined to
        the paragraph or bullet above until a blank line. */
    std::vector<Block> parse (const juce::String& markdown);

    /** Assets/Manual.md */
    juce::String text();
}

//==============================================================================
/**
 * The manual in a scrolling panel over its parent.
 */
class ManualOverlay : public ModalOverlay
{
public:
    explicit ManualOverlay (const juce::String& markdown = Manual::text());
    ~ManualOverlay() override;

    void show();

    void resized() override;

private:
    class Page;

    juce::TextButton closeButton { "Close" };
    juce::Viewport viewport;
    std::unique_ptr<Page> page;

    juce::Rectangle<int> getPanelBounds() const override   { return getLocalBounds().reduced (12); }
    void panelKeyPressed (const juce::KeyPress& key) override   { viewport.keyPressed (key); }
    void layOutPage();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManualOverlay)
};
