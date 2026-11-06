#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.h"

using TestHelpers::OverlayFixture;

//==============================================================================
// The same text and blocks as webui/test/manual.test.ts
TEST_CASE ("The manual parses headings, bullets and paragraphs", "[manual]")
{
    using Kind = Manual::Block::Kind;

    const auto blocks = Manual::parse ("# Title\r\n\r\nSome text\r\nover two lines.\r\n## Section\n"
                                       "- One\n  wrapped\n- Two\n\n#NotAHeading\n");

    const std::vector<Manual::Block> expected {
        { Kind::heading,    "Title" },
        { Kind::paragraph,  "Some text over two lines." },
        { Kind::subheading, "Section" },
        { Kind::bullet,     "One wrapped" },
        { Kind::bullet,     "Two" },
        { Kind::paragraph,  "#NotAHeading" },
    };

    CHECK (blocks == expected);
}

TEST_CASE_METHOD (OverlayFixture, "The manual overlay lays out when shown, scrolls from the keyboard, and closes on Escape", "[manual][ui]")
{
    juce::String markdown;
    for (int i = 0; i < 40; ++i)
        markdown << "## Section " << i << "\nSome text.\n\n";

    ManualOverlay overlay { markdown };
    parent.addChildComponent (overlay);

    juce::Viewport* viewport = nullptr;
    for (auto* child : overlay.getChildren())
        if (auto* v = dynamic_cast<juce::Viewport*> (child))
            viewport = v;

    REQUIRE (viewport != nullptr);
    auto* page = viewport->getViewedComponent();
    REQUIRE (page != nullptr);

    CHECK_FALSE (overlay.isVisible());
    CHECK (page->getHeight() == 0);

    overlay.show();
    CHECK (overlay.isVisible());
    CHECK (overlay.getBounds() == parent.getLocalBounds());
    CHECK (page->getWidth() <= viewport->getMaximumVisibleWidth());
    CHECK (page->getHeight() > viewport->getHeight());

    // The scroll bar moves at once; it tells the viewport asynchronously
    overlay.keyPressed (juce::KeyPress (juce::KeyPress::downKey));
    CHECK (viewport->getVerticalScrollBar().getCurrentRangeStart() > 0.0);

    overlay.keyPressed (juce::KeyPress (juce::KeyPress::escapeKey));
    CHECK_FALSE (overlay.isVisible());
}
