#pragma once

#include "../PluginProcessor.h"

//==============================================================================
/** The shared WebViewEditor, with Modulate's modulation, LFO curves and live state. The page side is webui/src/plugin/pluginBackend.ts. */
juce::AudioProcessorEditor* createWebViewEditor (AudioPluginAudioProcessor&);
