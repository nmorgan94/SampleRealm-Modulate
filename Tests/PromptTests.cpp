#include <catch2/catch_test_macros.hpp>

#include "TestHelpers.h"

using TestHelpers::OverlayFixture;

//==============================================================================
TEST_CASE_METHOD (OverlayFixture, "A prompt covers its parent and runs its callback on Return, not on Escape", "[prompt][ui]")
{
    PromptOverlay prompt;
    parent.addChildComponent (prompt);
    CHECK_FALSE (prompt.isVisible());

    int confirmed = 0;
    const auto confirm = [&] { prompt.askToConfirm ("Delete", "Delete it?", "Delete", [&] { ++confirmed; }); };

    confirm();
    CHECK (prompt.isVisible());
    CHECK (prompt.getBounds() == parent.getLocalBounds());

    CHECK (prompt.keyPressed (juce::KeyPress ('a')));   // swallowed
    CHECK (prompt.isVisible());

    CHECK (prompt.keyPressed (juce::KeyPress (juce::KeyPress::escapeKey)));
    CHECK_FALSE (prompt.isVisible());
    CHECK (confirmed == 0);

    confirm();
    CHECK (prompt.keyPressed (juce::KeyPress (juce::KeyPress::returnKey)));
    CHECK_FALSE (prompt.isVisible());
    CHECK (confirmed == 1);
}

TEST_CASE_METHOD (OverlayFixture, "A text prompt passes its text to the callback", "[prompt][ui]")
{
    PromptOverlay prompt;
    parent.addChildComponent (prompt);

    juce::String received;
    prompt.askForText ("Rename", "Old name", "Rename", [&] (const juce::String& text) { received = text; });
    prompt.keyPressed (juce::KeyPress (juce::KeyPress::returnKey));

    CHECK (received == "Old name");
}

TEST_CASE_METHOD (OverlayFixture, "A prompt grows to fit its message, wrapped lines included", "[prompt][ui]")
{
    PromptOverlay prompt;
    parent.addChildComponent (prompt);

    const auto messageHeight = [&] (const juce::String& message)
    {
        prompt.showMessage ("Title", message);

        for (auto* child : prompt.getChildren())
            if (auto* label = dynamic_cast<juce::Label*> (child); label != nullptr && label->getText() == message)
                return label->getHeight();

        return -1;
    };

    CHECK (messageHeight ("One") == 56);
    CHECK (messageHeight ("1\n2\n3\n4") == 56);
    CHECK (messageHeight ("1\n2\n3\n4\n5") == 60);
    CHECK (messageHeight (juce::String::repeatedString ("A long message that wraps ", 12)) > 60);
    CHECK (messageHeight ("Back to one") == 56);

    // Never taller than the editor
    CHECK (messageHeight (juce::String::repeatedString ("line\n", 40)) < parent.getHeight());
}
