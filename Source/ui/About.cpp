#include "About.h"
#include "../shared/BuildType.h"
#include <BuildInfo.h>

// The only file that includes BuildInfo.h, so a new commit recompiles just this
namespace About
{
    const char* const commit = PLUGIN_GIT_COMMIT;

    juce::String running (const juce::AudioProcessor& processor)
    {
        const juce::String format (juce::AudioProcessor::getWrapperTypeDescription (processor.wrapperType));
        const juce::PluginHostType host;

        return host.type == juce::PluginHostType::UnknownHost ? format : format + " in " + host.getHostDescription();
    }

    juce::String text (const juce::AudioProcessor& processor)
    {
        return juce::StringArray { "Version " JucePlugin_VersionString,
                                   juce::String (BuildType::name) + " build",
                                   "Commit " + juce::String (commit),
                                   running (processor),
                                   "Native UI" }.joinIntoString ("\n");
    }
}
