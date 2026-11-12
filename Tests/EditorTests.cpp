#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <EditorFixtures.h>
#include <samplerealm/BuildCommit.h>

#include "TestHelpers.h"
#include "PluginEditor.h"

using namespace TestHelpers;
using EditorFixtures::descendantsOf;
using Catch::Matchers::WithinAbs;

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "The editor's text is Orbitron", "[editor][font]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& lookAndFeel = editor.getLookAndFeel();

    const auto labels = descendantsOf<juce::Label> (editor);
    REQUIRE (labels.size() > 20);

    for (auto* label : labels)
    {
        CAPTURE (label->getText());
        CHECK (lookAndFeel.getLabelFont (*label).getTypefacePtr()->getName() == "Orbitron");
    }

    const auto boxes = descendantsOf<juce::ComboBox> (editor);
    REQUIRE (! boxes.empty());

    for (auto* box : boxes)
        CHECK (lookAndFeel.getComboBoxFont (*box).getTypefacePtr()->getName() == "Orbitron");

    const auto buttons = descendantsOf<juce::TextButton> (editor);
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

    const auto headers = descendantsOf<HeaderBar> (editor);
    REQUIRE (headers.size() == 1);

    const auto& layout = AudioPluginAudioProcessorEditor::headerLayout;
    CHECK (headers[0]->getBounds() == juce::Rectangle<int> (0, 0, AudioPluginAudioProcessorEditor::width, layout.height));
    CHECK (headers[0]->getTitle() == "SampleRealm: MODULATE");

    // Squeezed no further than HeaderBar's 70%, so not cut short
    const juce::Font titleFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::boldWeight).withHeight (layout.titleHeight));
    CHECK (juce::GlyphArrangement::getStringWidth (titleFont, headers[0]->getTitle()) * 0.7f <= static_cast<float> (layout.titleWidth));

    const auto bars = descendantsOf<PresetBar> (*headers[0]);
    REQUIRE (bars.size() == 1);
    CHECK (bars[0]->getX() > layout.edge + layout.titleWidth);

    const auto meters = descendantsOf<MasterMeter> (*headers[0]);
    REQUIRE (meters.size() == 1);
    CHECK (bars[0]->getRight() < meters[0]->getX());
}

TEST_CASE ("A labelled knob takes a typed value, snapped, and ignores words", "[editor]")
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    LabeledKnob<> knob;
    knob.setLabelText ("Release");
    auto& slider = knob.getSlider();
    slider.setRange (0.0, 5.0, 0.5);

    const auto labels = descendantsOf<juce::Label> (knob);
    REQUIRE (labels.size() == 1);
    CHECK (labels[0]->getText() == "Release");
    CHECK (slider.getTitle() == "Release");

    labels[0]->setText ("1.3", juce::sendNotificationSync);
    CHECK (slider.getValue() == 1.5);

    labels[0]->setText ("99", juce::sendNotificationSync);
    CHECK (slider.getValue() == 5.0);

    labels[0]->setText ("slow", juce::sendNotificationSync);
    CHECK (slider.getValue() == 5.0);
}

TEST_CASE_METHOD (ProcessorFixture, "A labelled choice knob takes only its names as typed values", "[editor]")
{
    const auto* id = ParameterSpecs::lfo[0].syncRate.id;
    LabeledKnob<ModulatableSlider> knob (processor, id);
    juce::AudioProcessorValueTreeState::SliderAttachment attachment (processor.getAPVTS(), id, knob.getSlider());
    auto& slider = knob.getSlider();

    const auto labels = descendantsOf<juce::Label> (knob);
    REQUIRE (labels.size() == 1);
    REQUIRE (slider.getValue() == 6.0);

    labels[0]->setText ("1/4", juce::sendNotificationSync);
    CHECK (slider.getValue() == 6.0);

    labels[0]->setText (" 2 bars ", juce::sendNotificationSync);
    CHECK (slider.getValue() == 13.0);

    labels[0]->setText ("2", juce::sendNotificationSync);
    CHECK (slider.getValue() == 13.0);
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
    CHECK (lines[2] == juce::String ("Commit ") + buildCommit);
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
