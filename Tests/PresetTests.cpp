#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    juce::MemoryBlock factoryPreset (const char* xml)
    {
        return { xml, std::strlen (xml) };
    }

    // Fixed factory presets, with user presets in a folder of their own
    struct PresetFixture : ProcessorFixture
    {
        ~PresetFixture()   { userDirectory.deleteRecursively(); }

        juce::File userDirectory = juce::File::createTempFile ("presets");

        PresetManager presets { processor.getAPVTS(), [this]
        {
            auto config = AudioPluginAudioProcessor::createPresetConfig();
            config.userDirectory = userDirectory;
            config.factoryPresets = {
                factoryPreset (R"(<PRESET name="Wobble" category="Bass"><Parameters>
                                    <PARAM id="filter_cutoff" value="400"/>
                                    <ModMatrix>
                                      <Assignment parameterID="filter_cutoff" sourceType="1" sourceIndex="2" minValue="0.2" maxValue="0.8"/>
                                    </ModMatrix>
                                    <LFOCurves>
                                      <LFOCurve index="2"><Point x="0" y="0"/><Point x="0.5" y="1"/><Point x="1" y="0"/></LFOCurve>
                                    </LFOCurves>
                                  </Parameters></PRESET>)"),
                factoryPreset (R"(<PRESET name="Sub" category="Bass"><Parameters><PARAM id="filter_cutoff" value="150"/></Parameters></PRESET>)"),
                factoryPreset (R"(<PRESET name="Bright" category="Lead"><Parameters><PARAM id="filter_cutoff" value="8000"/></Parameters></PRESET>)")
            };
            return config;
        }() };

        float cutoff()   { return getParameter (processor, "filter_cutoff"); }
    };

    juce::StringArray namesOf (const juce::Array<PresetManager::Preset>& list)
    {
        juce::StringArray names;

        for (const auto& preset : list)
            names.add (preset.name);

        return names;
    }

    const juce::StringArray factoryNames { "Sub", "Wobble", "Bright" };

    void assignCutoff (AudioPluginAudioProcessor& processor)
    {
        processor.getModulationManager().setAssignment ("filter_cutoff", { Modulation::Source::Envelope, 1, 0.1f, 0.9f });
    }
}

//==============================================================================
TEST_CASE_METHOD (PresetFixture, "Factory presets are listed by category, then name", "[presets]")
{
    CHECK (namesOf (presets.getFactoryPresets()) == factoryNames);
    CHECK (namesOf (presets.getPresets()) == factoryNames);
}

// The processor's own manager, with Assets/Presets embedded
TEST_CASE_METHOD (ProcessorFixture, "Every file in Assets/Presets is a factory preset that loads unmodified", "[presets]")
{
    const auto files = juce::File (__FILE__).getParentDirectory().getSiblingFile ("Assets/Presets")
                           .findChildFiles (juce::File::findFiles, false, "*.modulate");
    auto& presets = processor.getPresetManager();

    REQUIRE_FALSE (files.isEmpty());
    REQUIRE (presets.getFactoryPresets().size() == files.size());

    for (const auto& preset : presets.getFactoryPresets())
    {
        CHECK (presets.loadPreset (preset));
        CHECK (presets.getCurrent().name == preset.name);
        CHECK_FALSE (presets.isModified());
    }
}

TEST_CASE_METHOD (PresetFixture, "A loaded preset sets its values, modulation and curves, and shows its name", "[presets]")
{
    CHECK (presets.getCurrent().name == "Init");

    REQUIRE (presets.loadPreset (presets.find ("Wobble", true).value()));

    CHECK_THAT (cutoff(), WithinAbs (400.0, 1.0e-2));

    const auto assignment = processor.getModulationManager().getAssignment ("filter_cutoff");
    CHECK (assignment.isLFO());
    CHECK (assignment.sourceIndex == 2);
    CHECK_THAT (assignment.maxValue, WithinAbs (0.8, 1.0e-6));
    CHECK (processor.getLFOCurvePoints (2).size() == 3);

    CHECK (presets.getCurrent().name == "Wobble");
    CHECK (presets.getCurrent().category == "Bass");
    CHECK (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "A preset without modulation or curves clears them", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Wobble", true).value()));
    REQUIRE (presets.loadPreset (presets.find ("Sub", true).value()));

    CHECK_FALSE (processor.getModulationManager().getAssignment ("filter_cutoff").isAssigned());
    CHECK (processor.getLFOCurvePoints (2).empty());
}

TEST_CASE_METHOD (PresetFixture, "Moving a parameter marks the preset modified until the next load", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Bright", true).value()));

    setParameter (processor, "filter_cutoff", 1000.0f);
    CHECK (presets.isModified());

    REQUIRE (presets.loadPreset (presets.find ("Bright", true).value()));
    CHECK_FALSE (presets.isModified());
    CHECK_THAT (cutoff(), WithinAbs (8000.0, 1.0e-1));
}

// The processor's own manager, which hears about modulation and curves
TEST_CASE_METHOD (ProcessorFixture, "Changing modulation or a curve marks the preset modified", "[presets]")
{
    auto& presets = processor.getPresetManager();
    presets.loadInit();
    REQUIRE_FALSE (presets.isModified());

    SECTION ("Modulation")
    {
        assignCutoff (processor);
        CHECK (presets.isModified());
    }

    SECTION ("A curve")
    {
        processor.setLFOCurvePoints (0, { { 0.0f, 0.0f }, { 1.0f, 1.0f } });
        CHECK (presets.isModified());
    }
}

TEST_CASE_METHOD (PresetFixture, "Init restores the defaults and clears modulation and curves", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Wobble", true).value()));
    assignCutoff (processor);

    presets.loadInit();

    CHECK_THAT (cutoff(), WithinAbs (ParameterSpecs::filterCutoff.defaultValue, 1.0e-2));
    CHECK_FALSE (processor.getModulationManager().getAssignment ("filter_cutoff").isAssigned());
    CHECK (processor.getLFOCurvePoints (2).empty());
    CHECK (presets.getCurrent().name == "Init");
    CHECK_FALSE (presets.isModified());

    // The managers follow the new state, so edits after Init still land in it
    assignCutoff (processor);
    CHECK (processor.getAPVTS().state.getChildWithName ("ModMatrix").getNumChildren() == 1);
}

TEST_CASE_METHOD (PresetFixture, "Replacing the state moves the editor's counter", "[presets]")
{
    const auto before = processor.getStateChanges();
    presets.loadInit();
    CHECK (processor.getStateChanges() != before);
}

TEST_CASE_METHOD (PresetFixture, "Next and previous wrap, and start from either end after Init", "[presets]")
{
    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == factoryNames[factoryNames.size() - 1]);

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    presets.loadInit();
    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == factoryNames[factoryNames.size() - 1]);
    CHECK_THAT (cutoff(), WithinAbs (8000.0, 1.0e-1));
}

TEST_CASE_METHOD (PresetFixture, "Next and previous step past a preset that won't load", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("A", {}).wasOk());
    REQUIRE (userDirectory.getChildFile ("Broken.modulate").replaceWithText ("not a preset"));

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == "A");
}

TEST_CASE_METHOD (PresetFixture, "Rescanning moves the change counter only when the folder changed", "[presets]")
{
    presets.refreshUserPresets();
    const auto before = presets.getChangeCounter();

    presets.refreshUserPresets();
    CHECK (presets.getChangeCounter() == before);

    REQUIRE (userDirectory.createDirectory().wasOk());
    REQUIRE (userDirectory.getChildFile ("Mine.modulate").replaceWithText ("<PRESET/>"));
    presets.refreshUserPresets();
    CHECK (presets.getChangeCounter() != before);
}

TEST_CASE_METHOD (PresetFixture, "User presets save, rename and delete in their folder", "[presets]")
{
    setParameter (processor, "filter_cutoff", 3000.0f);
    assignCutoff (processor);

    REQUIRE (presets.saveUserPreset ("Mine", "Lead").wasOk());
    const auto mine = userDirectory.getChildFile ("Mine.modulate");
    CHECK (mine.existsAsFile());
    CHECK (presets.getCurrent().name == "Mine");
    CHECK_FALSE (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());

    // Listed after the factory presets
    REQUIRE (presets.getPresets().getLast().file == mine);

    presets.loadInit();
    REQUIRE (presets.loadPreset (presets.getPresets().getLast()));
    CHECK_THAT (cutoff(), WithinAbs (3000.0, 1.0e-1));
    CHECK (processor.getModulationManager().getAssignment ("filter_cutoff").isEnvelope());
    CHECK (presets.getCurrent().name == "Mine");

    REQUIRE (presets.renameUserPreset (presets.getPresets().getLast(), "Yours").wasOk());
    const auto yours = userDirectory.getChildFile ("Yours.modulate");
    CHECK_FALSE (mine.exists());
    CHECK (yours.existsAsFile());
    CHECK (presets.getCurrent().name == "Yours");
    CHECK (namesOf (presets.getUserPresets()) == juce::StringArray { "Yours" });

    REQUIRE (presets.deleteUserPreset (presets.getPresets().getLast()).wasOk());
    CHECK_FALSE (yours.exists());
    CHECK (presets.getUserPresets().isEmpty());
    CHECK (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "User presets can't take a factory name", "[presets]")
{
    CHECK (presets.saveUserPreset ("wobble", {}).failed());
    CHECK (presets.saveUserPreset ("   ", {}).failed());
    CHECK_FALSE (userDirectory.getChildFile ("wobble.modulate").exists());

    REQUIRE (presets.saveUserPreset ("Mine", {}).wasOk());
    CHECK (presets.renameUserPreset (presets.getUserPresets().getFirst(), "Sub").failed());
    CHECK (userDirectory.getChildFile ("Mine.modulate").existsAsFile());
}

TEST_CASE_METHOD (PresetFixture, "State from before presets existed shows Init, modified", "[presets][state]")
{
    REQUIRE (presets.loadPreset (presets.find ("Bright", true).value()));

    juce::ValueTree state (processor.getAPVTS().state.getType());
    state.appendChild ({ "PARAM", { { "id", "filter_cutoff" }, { "value", 250.0f } } }, nullptr);
    presets.restoreState (state);

    CHECK_THAT (cutoff(), WithinAbs (250.0, 1.0e-2));
    CHECK (presets.getCurrent().name == "Init");
    CHECK (presets.isModified());
}

TEST_CASE_METHOD (ProcessorFixture, "A reopened session shows the same preset", "[presets][state]")
{
    auto& presets = processor.getPresetManager();
    presets.loadInit();
    setParameter (processor, "filter_cutoff", 2000.0f);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    const auto current = restored.getPresetManager().getCurrent();
    CHECK (current.name == "Init");
    CHECK (current.isFactory);
    CHECK (restored.getPresetManager().isModified());
    CHECK_THAT (getParameter (restored, "filter_cutoff"), WithinAbs (2000.0, 1.0e-1));

    // Kept by the preset manager, not left on the parameters
    CHECK_FALSE (restored.getAPVTS().state.hasProperty ("presetName"));
}
