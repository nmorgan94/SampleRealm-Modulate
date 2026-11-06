#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "PluginEditor.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    template <typename ComponentType>
    void findAll (juce::Component& parent, std::vector<ComponentType*>& found)
    {
        for (auto* child : parent.getChildren())
        {
            if (auto* match = dynamic_cast<ComponentType*> (child))
                found.push_back (match);

            findAll (*child, found);
        }
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "The editor's text is Orbitron", "[editor][font]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& lookAndFeel = editor.getLookAndFeel();

    std::vector<juce::Label*> labels;
    findAll (editor, labels);
    REQUIRE (labels.size() > 20);

    for (auto* label : labels)
    {
        CAPTURE (label->getText());
        CHECK (lookAndFeel.getLabelFont (*label).getTypefacePtr()->getName() == "Orbitron");
    }

    std::vector<juce::ComboBox*> boxes;
    findAll (editor, boxes);
    REQUIRE (! boxes.empty());

    for (auto* box : boxes)
        CHECK (lookAndFeel.getComboBoxFont (*box).getTypefacePtr()->getName() == "Orbitron");

    std::vector<juce::TextButton*> buttons;
    findAll (editor, buttons);
    REQUIRE (buttons.size() >= 5);

    for (auto* button : buttons)
    {
        CAPTURE (button->getButtonText());
        CHECK (lookAndFeel.getTextButtonFont (*button, button->getHeight()).getTypefacePtr()->getName() == "Orbitron");
    }
}

TEST_CASE_METHOD (ProcessorFixture, "The header fits the title, the preset bar and the meter side by side", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);

    std::vector<HeaderBar*> headers;
    findAll (editor, headers);
    REQUIRE (headers.size() == 1);
    CHECK (headers[0]->getBounds() == juce::Rectangle<int> (0, 0, AudioPluginAudioProcessorEditor::width, HeaderBar::height));

    std::vector<juce::Label*> labels;
    findAll (*headers[0], labels);
    const auto title = std::ranges::find_if (labels, [] (auto* label) { return label->getText() == "SampleRealm: MODULATE"; });
    REQUIRE (title != labels.end());

    // Squeezed no further than the label allows
    const auto titleWidth = juce::GlyphArrangement::getStringWidth (editor.getLookAndFeel().getLabelFont (**title), (*title)->getText());
    CHECK (titleWidth * (*title)->getMinimumHorizontalScale() <= static_cast<float> ((*title)->getWidth()));

    std::vector<PresetBar*> bars;
    findAll (*headers[0], bars);
    REQUIRE (bars.size() == 1);
    CHECK (bars[0]->getX() > (*title)->getRight());

    std::vector<MasterMeter*> meters;
    findAll (*headers[0], meters);
    REQUIRE (meters.size() == 1);
    CHECK (bars[0]->getRight() < meters[0]->getX());
}

TEST_CASE ("Orbitron's weights come from the one variable font", "[editor][font]")
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const auto weightOf = [] (const juce::FontOptions& options)
    {
        const auto variables = options.getTypeface()->getConfiguredVariables();
        REQUIRE (variables.size() == 1);
        CHECK (variables.front().tag == juce::FontFeatureTag ("wght"));
        return variables.front().value;
    };

    const auto regular = CustomLookAndFeel::orbitron (CustomLookAndFeel::regularWeight);
    const auto bold = CustomLookAndFeel::orbitron (CustomLookAndFeel::boldWeight);

    CHECK (regular.getTypeface()->getName() == "Orbitron");
    CHECK_THAT (weightOf (regular), WithinAbs (400.0, 0.0));
    CHECK_THAT (weightOf (bold), WithinAbs (700.0, 0.0));
}

TEST_CASE_METHOD (ProcessorFixture, "About shows the version and build type, the commit and the UI", "[editor]")
{
    const auto lines = juce::StringArray::fromLines (About::text (processor));

    REQUIRE (lines.size() == 5);
    CHECK (lines[0] == "Version " JucePlugin_VersionString);
   #if JUCE_DEBUG
    CHECK (lines[1] == "Debug build");
   #else
    CHECK (lines[1] == "Release build");
   #endif
    CHECK (lines[2] == juce::String ("Commit ") + About::commit);
    CHECK (lines[3] == "Undefined");
    CHECK (lines[4] == "Native UI");
}

// For comparing with the web editor by eye: Modulate_Tests "[.snapshot]" writes editor-snapshot.png here
TEST_CASE_METHOD (ProcessorFixture, "Write a snapshot of the editor", "[.snapshot]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    const auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, 2.0f);

    auto file = juce::File::getCurrentWorkingDirectory().getChildFile ("editor-snapshot.png");
    file.deleteFile();
    juce::FileOutputStream stream (file);
    REQUIRE (juce::PNGImageFormat().writeImageToStream (image, stream));
}
