#include "ModulateWebView.h"
#include "../PluginEditor.h"
#include <webview_editor/webview_editor.h>

namespace
{
    // { "filter_cutoff": { source: "lfo", index, min, max }, ... }, for the assigned parameters
    juce::var toVar (const ModulateSynth::Assignments& assignments)
    {
        auto byId = std::make_unique<juce::DynamicObject>();

        for (size_t i = 0; i < assignments.size(); ++i)
        {
            const auto& assignment = assignments[i];

            if (! assignment.isAssigned())
                continue;

            auto entry = std::make_unique<juce::DynamicObject>();
            entry->setProperty ("source", assignment.isLFO() ? "lfo" : "env");
            entry->setProperty ("index", assignment.sourceIndex);
            entry->setProperty ("min", assignment.minValue);
            entry->setProperty ("max", assignment.maxValue);
            byId->setProperty (ParameterSpecs::all[i]->id, juce::var (entry.release()));
        }

        return juce::var (byId.release());
    }

    // Each LFO's points as { x, y }, none for the default shape
    juce::var toVar (const ModulateSynth::Curves& curves)
    {
        juce::Array<juce::var> lfos;

        for (const auto& curve : curves)
        {
            juce::Array<juce::var> points;

            for (const auto& point : curve.view())
            {
                auto entry = std::make_unique<juce::DynamicObject>();
                entry->setProperty ("x", point.x);
                entry->setProperty ("y", point.y);
                points.add (juce::var (entry.release()));
            }

            lfos.add (points);
        }

        return lfos;
    }

    juce::var toVar (const ModulateSynth::Live& state)
    {
        const auto values = [] (const auto& atomics)
        {
            juce::Array<juce::var> list;

            for (const auto& value : atomics)
                list.add (value.load());

            return list;
        };

        juce::Array<juce::var> stages;

        for (const auto& stage : state.envelopeStage)
            stages.add (std::to_underlying (stage.load()));

        auto live = std::make_unique<juce::DynamicObject>();
        live->setProperty ("lfoPhase", values (state.lfoPhase));
        live->setProperty ("lfoValue", values (state.lfoValue));
        live->setProperty ("envelopeLevel", values (state.envelopeLevel));
        live->setProperty ("envelopeProgress", values (state.envelopeProgress));
        live->setProperty ("envelopeStage", stages);
        return juce::var (live.release());
    }

    // What the page has, so it hears only about changes it didn't make
    struct SentState
    {
        ModulateSynth::Assignments modulation;
        ModulateSynth::Curves curves;
    };
}

juce::AudioProcessorEditor* createWebViewEditor (AudioPluginAudioProcessor& processor)
{
    auto sent = std::make_shared<SentState> (processor.getModulationManager().getAssignments(), processor.getLFOCurves());

    const auto withOptions = [&processor, sent] (juce::WebBrowserComponent::Options options)
    {
        auto& modulation = processor.getModulationManager();

        return options
            .withInitialisationData ("modulation", toVar (sent->modulation))
            .withInitialisationData ("curves", toVar (sent->curves))
            // Source 1 is an LFO, 2 an envelope, and 0 clears it
            .withNativeFunction ("setModulation", [sent, &modulation] (const auto& args, auto complete)
            {
                const auto id = args[0].toString();
                modulation.setAssignment (id, { Modulation::sourceFromNumber (args[1]), args[2], args[3], args[4] });

                // The page has this edit already
                if (const auto index = Parameters::indexOf (id))
                    sent->modulation[*index] = modulation.getAssignment (id);

                complete ({});
            })
            // The points as x, y pairs
            .withNativeFunction ("setCurve", [sent, &processor] (const auto& args, auto complete)
            {
                const int lfo = args[0];
                std::vector<LfoCurve::Point> points;

                if (const auto* values = args[1].getArray())
                    for (int i = 0; i + 1 < values->size(); i += 2)
                        points.emplace_back (values->getUnchecked (i), values->getUnchecked (i + 1));

                if (juce::isPositiveAndBelow (lfo, ModulateSynth::numLfos))
                {
                    const auto index = static_cast<size_t> (lfo);
                    processor.setLFOCurvePoints (index, points);
                    sent->curves[index] = processor.getLFOCurves()[index];
                }

                complete ({});
            });
    };

    // Whatever changed them: the page, a preset, or the host restoring a session
    const auto onTick = [&processor, sent] (const WebViewEditor::EmitEvent& emit)
    {
        emit ("live", toVar (processor.getLive()));

        if (const auto modulation = processor.getModulationManager().getAssignments(); modulation != sent->modulation)
        {
            sent->modulation = modulation;
            emit ("modulation", toVar (modulation));
        }

        if (const auto curves = processor.getLFOCurves(); curves != sent->curves)
        {
            sent->curves = curves;
            emit ("curves", toVar (curves));
        }
    };

    return new WebViewEditor (processor, { .specs = ParameterSpecs::all,
                                           .parameters = processor.getAPVTS(),
                                           .presets = &processor.getPresetManager(),
                                           .meter = &processor.getOutputMeter(),
                                           .width = AudioPluginAudioProcessorEditor::width,
                                           .height = AudioPluginAudioProcessorEditor::height,
                                           .background = juce::Colours::black,
                                           .onTick = onTick,
                                           .withOptions = withOptions });
}
