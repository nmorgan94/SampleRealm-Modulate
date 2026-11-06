#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "shared/SynthSettings.h"
#include <BinaryData.h>

#if PLUGIN_WEBVIEW_EDITOR
 #include "webview/WebViewEditor.h"
#endif

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", Parameters::createLayout())
{
    reinitialiseStateManagers();
    apvts.state.addListener (this);

    // Parameters reach the PresetManager as parameters; modulation and curves only as state
    modulationManager.onEdit = lfoCurveState.onEdit = [this] { presetManager.markModified(); };
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    apvts.state.removeListener (this);
}

PresetManager::Config AudioPluginAudioProcessor::createPresetConfig()
{
    PresetManager::Config config;
    config.fileExtension = ".modulate";
    config.userDirectory = PresetManager::getDefaultUserDirectory (JucePlugin_Manufacturer, JucePlugin_Name);
    config.author        = JucePlugin_Manufacturer;
    config.formatVersion = Parameters::versionHint;

    // Factory presets are embedded from Assets/Presets alongside the font
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const auto* resource = BinaryData::namedResourceList[i];

        if (! juce::String (BinaryData::getNamedResourceOriginalFilename (resource)).endsWithIgnoreCase (config.fileExtension))
            continue;

        int size = 0;

        if (const auto* data = BinaryData::getNamedResource (resource, size))
            config.factoryPresets.emplace_back (data, static_cast<size_t> (size));
    }

    return config;
}

//==============================================================================
float AudioPluginAudioProcessor::getModulatedParam(const juce::String& paramID) const
{
    const auto index = Parameters::indexOf(paramID);
    if (! index)
        return 0.0f;

    const auto assignment = modulationManager.getAssignments()[*index];

    return ModulateSynth::modulatedValue(*ParameterSpecs::all[*index], apvts.getRawParameterValue(paramID)->load(),
                                         assignment, getLive().sourceValue(assignment));
}

ModulateSynth::Settings AudioPluginAudioProcessor::currentSettings() const
{
    return makeSynthSettings([this](const ParameterSpecs::Spec& spec) { return apvts.getRawParameterValue(spec.id)->load(); },
                             modulationManager.getAssignments(), lfoCurveState.getCurves(), tempo);
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    outputMeter.setNumChannels(getTotalNumOutputChannels());
    outputMeter.reset();

    synth.setSettings(currentSettings());
    synth.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            const auto timeSig = position->getTimeSignature().orFallback(juce::AudioPlayHead::TimeSignature());
            tempo = { position->getBpm().orFallback(120.0), timeSig.numerator, timeSig.denominator };
        }
    }

    synth.setSettings(currentSettings());
    synth.process(buffer, midiMessages);

    outputMeter.process(buffer);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
   #if PLUGIN_WEBVIEW_EDITOR
    return new WebViewEditor (*this);
   #else
    return new AudioPluginAudioProcessorEditor (*this);
   #endif
}

//==============================================================================
void AudioPluginAudioProcessor::reinitialiseStateManagers()
{
    auto modMatrixState = getOrCreateStateChild(apvts.state, modMatrixStateID);
    auto lfoCurvesState = getOrCreateStateChild(apvts.state, lfoCurvesStateID);

    modulationManager.initialise(modMatrixState, &undoManager);
    lfoCurveState.initialise(lfoCurvesState, &undoManager);
}

// apvts.replaceState: a preset, Init or a session. Init's state has no ModMatrix or LFOCurves
void AudioPluginAudioProcessor::valueTreeRedirected (juce::ValueTree&)
{
    reinitialiseStateManagers();
    ++stateChanges;
}

void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    presetManager.addPresetInfoTo (state);

    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml != nullptr)
        presetManager.restoreState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}


//==============================================================================
juce::ValueTree AudioPluginAudioProcessor::getOrCreateStateChild(juce::ValueTree parent, const juce::Identifier& childID)
{
    auto child = parent.getChildWithName(childID);
    if (! child.isValid())
    {
        child = juce::ValueTree(childID);
        parent.addChild(child, -1, nullptr);
    }
    
    return child;
}
