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
