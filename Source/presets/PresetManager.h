#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
 * Factory and user presets for any APVTS-based plugin.
 */
class PresetManager : private juce::AudioProcessorListener
{
public:
    struct Config
    {
        juce::String fileExtension = ".preset"; // including the dot
        juce::File userDirectory;
        juce::String author;
        int formatVersion = 1;
        std::vector<juce::MemoryBlock> factoryPresets;

        // Only presets of the current type (or none) are listed, and Init keeps it
        juce::String typeParameterId;
    };

    struct Preset
    {
        juce::String name, category, type;
        bool isFactory = false;
        std::size_t factoryIndex = 0; // into Config::factoryPresets
        juce::File file;              // user presets only

        bool operator== (const Preset&) const = default;
    };

    struct CurrentPreset
    {
        juce::String name { "Init" }, category;
        bool isFactory = true;

        bool matches (const Preset& p) const   { return p.name == name && p.isFactory == isFactory; }
    };

    PresetManager (juce::AudioProcessorValueTreeState&, Config);
    ~PresetManager() override;

    static juce::File getDefaultUserDirectory (const juce::String& company, const juce::String& product);

    //==============================================================================
    const juce::Array<Preset>& getFactoryPresets() const noexcept   { return factoryPresets; }
    /** Scans the user folder the first time it's called. */
    const juce::Array<Preset>& getUserPresets() const;

    /** Rescans the user folder. Save, rename and delete do this automatically; call it
        before showing a preset list to pick up files changed outside the plugin. */
    void refreshUserPresets();

    /** Factory then user presets, only of the current type when types are in use. */
    juce::Array<Preset> getPresets() const;
    std::optional<Preset> find (const juce::String& name, bool isFactory) const;
    juce::File getUserDirectory() const                             { return config.userDirectory; }
    /** Opens the user folder, creating it first. */
    void showUserDirectory() const;

    /** Resets everything but the type parameter to defaults, named "Init". */
    void loadInit();
    bool loadPreset (const Preset&);
    bool loadNext();
    bool loadPrevious();

    bool userPresetExists (const juce::String& name) const;
    juce::Result saveUserPreset (const juce::String& name, const juce::String& category);
    juce::Result renameUserPreset (const Preset&, const juce::String& newName);
    juce::Result deleteUserPreset (const Preset&);

    //==============================================================================
    CurrentPreset getCurrent() const;
    std::optional<Preset> getCurrentPreset() const;

    bool isModified() const noexcept                 { return modified.load(); }
    void markModified() noexcept;
    juce::uint32 getChangeCounter() const noexcept   { return changeCounter.load(); }

    //==============================================================================
    /** For getStateInformation: stores the current preset name and modified flag
        on a copy of the state, so sessions reopen showing the same preset. */
    void addPresetInfoTo (juce::ValueTree& stateCopy) const;

    /** For setStateInformation and preset loading: replaces the APVTS state (missing
        parameters get their defaults) and restores the preset info. */
    void restoreState (juce::ValueTree newState);

private:
    juce::AudioProcessorValueTreeState& apvts;
    const Config config;
    juce::Array<Preset> factoryPresets;

    // Scanned lazily, so plugin instances that never show a preset list don't read the folder
    mutable juce::Array<Preset> userPresets;
    mutable bool userPresetsScanned = false;

    juce::CriticalSection currentLock;
    CurrentPreset current;

    std::atomic<bool> modified { false }, isLoading { false };
    std::atomic<juce::uint32> changeCounter { 0 };

    juce::File getUserFile (const juce::String& name) const;
    juce::Result checkUserName (const juce::String& trimmedName) const;
    void scanUserPresets() const;
    juce::String getCurrentType() const;
    std::unique_ptr<juce::XmlElement> createPresetXml (const juce::String& name, const juce::String& category) const;
    void setCurrent (const CurrentPreset&, bool isModified);
    bool step (int delta);

    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override;
    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override {}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
