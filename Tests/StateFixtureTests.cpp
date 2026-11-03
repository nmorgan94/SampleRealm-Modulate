#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    // Saved by 0.0.1, with every parameter moved, two assignments and a drawn LFO 2
    std::unique_ptr<juce::XmlElement> savedSession()
    {
        auto xml = juce::XmlDocument::parse (juce::File (__FILE__).getSiblingFile ("fixtures/session-0.0.1.xml"));
        REQUIRE (xml != nullptr);
        return xml;
    }

    void load (AudioPluginAudioProcessor& processor, const juce::XmlElement& xml)
    {
        juce::MemoryBlock state;
        juce::AudioProcessor::copyXmlToBinary (xml, state);
        processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "A session saved by 0.0.1 loads the same", "[state]")
{
    const auto session = savedSession();
    load (processor, *session);

    SECTION ("Every saved parameter value")
    {
        int numParams = 0;

        for (auto* param : session->getChildWithTagNameIterator ("PARAM"))
        {
            const auto id = param->getStringAttribute ("id");
            CAPTURE (id);

            auto* value = processor.getAPVTS().getRawParameterValue (id);
            REQUIRE (value != nullptr);
            CHECK_THAT (value->load(), WithinAbs (param->getDoubleAttribute ("value"), 1.0e-6));
            ++numParams;
        }

        CHECK (numParams == processor.getParameters().size());
    }

    SECTION ("Modulation assignments")
    {
        const auto cutoff = processor.getModulationManager().getAssignment ("filter_cutoff");
        CHECK (cutoff.isLFO());
        CHECK (cutoff.sourceIndex == 2);
        CHECK_THAT (cutoff.minValue, WithinAbs (0.2, 1.0e-6));
        CHECK_THAT (cutoff.maxValue, WithinAbs (0.8, 1.0e-6));

        const auto morph = processor.getModulationManager().getAssignment ("osc1_morph");
        CHECK (morph.isEnvelope());
        CHECK (morph.sourceIndex == 1);
        CHECK_THAT (morph.minValue, WithinAbs (0.1, 1.0e-6));
        CHECK_THAT (morph.maxValue, WithinAbs (0.6, 1.0e-6));

        CHECK_FALSE (processor.getModulationManager().getAssignment ("osc2_morph").isAssigned());
    }

    SECTION ("LFO curves")
    {
        const auto points = processor.getLFOCurvePoints (1);
        REQUIRE (points.size() == 4);
        CHECK_THAT (points[1].x, WithinAbs (0.3, 1.0e-6));
        CHECK_THAT (points[1].y, WithinAbs (0.9, 1.0e-6));
        CHECK_THAT (points[2].x, WithinAbs (0.65, 1.0e-6));
        CHECK_THAT (points[2].y, WithinAbs (0.1, 1.0e-6));

        CHECK (processor.getLFOCurvePoints (0).empty());
    }

    SECTION ("Saving it again gives the same session")
    {
        juce::MemoryBlock state;
        processor.getStateInformation (state);
        const auto resaved = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));

        REQUIRE (resaved != nullptr);
        CHECK (resaved->isEquivalentTo (session.get(), true));
    }
}
