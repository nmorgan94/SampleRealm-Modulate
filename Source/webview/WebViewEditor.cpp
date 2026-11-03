// The page side of these names is webui/src/plugin/juceBackend.ts
#include "WebViewEditor.h"
#include "../PluginEditor.h"
#include "../shared/ParameterSpecs.h"
#include "../shared/UiInfoJson.h"
#include "../ui/MeterBallistics.h"
#include <WebUIData.h>

#if JUCE_MAC
 #import <Foundation/Foundation.h>
#endif

namespace
{
    const auto background = juce::Colours::black;

    // The page's INIT_PRESET
    const juce::String initPreset { "Init" };

    const char* mimeTypeFor (const juce::String& fileName)
    {
        static const std::unordered_map<juce::String, const char*> types {
            { "html", "text/html" },
            { "js",   "text/javascript" },
            { "css",  "text/css" },
            { "ttf",  "font/ttf" },
            { "json", "application/json" }
        };

        const auto it = types.find (fileName.fromLastOccurrenceOf (".", false, false).toLowerCase());
        return it != types.end() ? it->second : "application/octet-stream";
    }

    std::vector<std::byte> readAll (juce::InputStream& stream)
    {
        std::vector<std::byte> bytes (static_cast<size_t> (stream.getTotalLength()));
        stream.read (bytes.data(), static_cast<int> (bytes.size()));
        return bytes;
    }

    struct SinglePageBrowser final : juce::WebBrowserComponent
    {
        using WebBrowserComponent::WebBrowserComponent;

        bool pageAboutToLoad (const juce::String& newURL) override
        {
            return newURL == getResourceProviderRoot();
        }
    };

    // The Mac web view is white until the page draws, and JUCE has no option for it
    void hideMacWebViewBackground ([[maybe_unused]] juce::WebBrowserComponent& browser)
    {
       #if JUCE_MAC
        for (auto* child : browser.getChildren())
            if (auto* host = dynamic_cast<juce::NSViewComponent*> (child))
                [(id) host->getView() setValue: @NO forKey: @"drawsBackground"];
       #endif
    }

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
}

//==============================================================================
WebViewEditor::WebViewEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      uiZipStream (WebUIData::WebUI_zip, static_cast<size_t> (WebUIData::WebUI_zipSize), false)
{
    for (const auto* spec : ParameterSpecs::all)
        relays.push_back (std::make_unique<juce::WebSliderRelay> (spec->id));

    sentModulation = processorRef.getModulationManager().getAssignments();
    sentCurves = processorRef.getLFOCurves();

    browser = std::make_unique<SinglePageBrowser> (createOptions());
    hideMacWebViewBackground (*browser);

    auto& apvts = processorRef.getAPVTS();

    for (size_t i = 0; i < relays.size(); ++i)
        attachments.push_back (std::make_unique<juce::WebSliderParameterAttachment> (
            *apvts.getParameter (ParameterSpecs::all[i]->id), *relays[i], nullptr));

    // Drop anything played while no meter was showing
    processorRef.getOutputMeter().reset();

    addAndMakeVisible (*browser);
    browser->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (AudioPluginAudioProcessorEditor::width, AudioPluginAudioProcessorEditor::height);
    startTimerHz (MeterBallistics::updateHz);
}

juce::WebBrowserComponent::Options WebViewEditor::createOptions()
{
    auto options = juce::WebBrowserComponent::Options{}
                       .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                       .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                                                    .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory))
                                                    .withBackgroundColour (background))
                       .withNativeIntegrationEnabled()
                       .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (auto& relay : relays)
        options = options.withOptionsFrom (*relay);

    auto& modulation = processorRef.getModulationManager();

    return options
        .withInitialisationData ("uiInfo", juce::String (uiInfoJson (JucePlugin_VersionString)))
        .withInitialisationData ("modulation", toVar (sentModulation))
        .withInitialisationData ("curves", toVar (sentCurves))
        .withNativeFunction ("presetList", [this] (const auto&, auto complete)
        {
            auto result = std::make_unique<juce::DynamicObject>();
            result->setProperty ("names", presets.getPresetList());
            result->setProperty ("current", presets.getCurrentPreset());
            complete (juce::var (result.release()));
        })
        .withNativeFunction ("presetLoad", [this] (const auto& args, auto complete)
        {
            const auto name = args[0].toString();

            if (name != initPreset)
            {
                complete (presets.loadPreset (name));
                return;
            }

            presets.loadInit();
            complete (true);
        })
        .withNativeFunction ("presetSave", [this] (const auto& args, auto complete)
        {
            complete (presets.savePreset (args[0].toString()));
        })
        .withNativeFunction ("presetDelete", [this] (const auto& args, auto complete)
        {
            complete (presets.deletePreset (args[0].toString()));
        })
        // Source 1 is an LFO, 2 an envelope, and 0 clears it
        .withNativeFunction ("setModulation", [this, &modulation] (const auto& args, auto complete)
        {
            const auto id = args[0].toString();
            modulation.setAssignment (id, { Modulation::sourceFromNumber (args[1]), args[2], args[3], args[4] });

            // The page has this edit already
            if (const auto index = Parameters::indexOf (id))
                sentModulation[*index] = modulation.getAssignment (id);

            complete ({});
        })
        // The points as x, y pairs
        .withNativeFunction ("setCurve", [this] (const auto& args, auto complete)
        {
            const int lfo = args[0];
            std::vector<LfoCurve::Point> points;

            if (const auto* values = args[1].getArray())
                for (int i = 0; i + 1 < values->size(); i += 2)
                    points.emplace_back (values->getUnchecked (i), values->getUnchecked (i + 1));

            if (juce::isPositiveAndBelow (lfo, ModulateSynth::numLfos))
            {
                const auto index = static_cast<size_t> (lfo);
                processorRef.setLFOCurvePoints (index, points);
                sentCurves[index] = processorRef.getLFOCurves()[index];
            }

            complete ({});
        });
}

std::optional<juce::WebBrowserComponent::Resource> WebViewEditor::getResource (const juce::String& url)
{
    const auto path = url == "/" ? juce::String ("index.html") : url.fromFirstOccurrenceOf ("/", false, false);

    if (auto* entry = uiZip.getEntry (path))
    {
        const std::unique_ptr<juce::InputStream> stream (uiZip.createStreamForEntry (*entry));

        if (stream != nullptr)
            return juce::WebBrowserComponent::Resource { readAll (*stream), mimeTypeFor (path) };
    }

    return std::nullopt;
}

void WebViewEditor::timerCallback()
{
    auto& meter = processorRef.getOutputMeter();

    juce::Array<juce::var> peaks;
    bool silent = true;

    for (int channel = 0; channel < PeakMeter::maxChannels; ++channel)
    {
        const auto peak = meter.readAndReset (channel);
        silent = silent && peak == 0.0f;
        peaks.add (peak);
    }

    // Silence only needs sending once
    if (! (silent && lastSilent))
    {
        auto level = std::make_unique<juce::DynamicObject>();
        level->setProperty ("peaks", peaks);
        browser->emitEventIfBrowserIsVisible ("level", juce::var (level.release()));
    }

    lastSilent = silent;

    browser->emitEventIfBrowserIsVisible ("live", toVar (processorRef.getLive()));
    sendStateIfChanged();
}

void WebViewEditor::sendStateIfChanged()
{
    // Held until the page can hear it
    if (! browser->isShowing())
        return;

    const auto modulation = processorRef.getModulationManager().getAssignments();

    if (modulation != sentModulation)
    {
        sentModulation = modulation;
        browser->emitEventIfBrowserIsVisible ("modulation", toVar (modulation));
    }

    const auto curves = processorRef.getLFOCurves();

    if (curves != sentCurves)
    {
        sentCurves = curves;
        browser->emitEventIfBrowserIsVisible ("curves", toVar (curves));
    }
}

void WebViewEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
}

void WebViewEditor::resized()
{
    browser->setBounds (getLocalBounds());
}
