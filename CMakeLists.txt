cmake_minimum_required(VERSION 3.25)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)

# Change to your project name
project(Modulate)

# Enables `ctest`
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_XCODE_GENERATE_SCHEME OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CPM_DOWNLOAD_VERSION 0.43.1)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif ()

include(${CPM_DOWNLOAD_LOCATION})

# Bring in JUCE locally
CPMAddPackage(
  NAME
    JUCE
  GIT_TAG
    9.0.2
  GITHUB_REPOSITORY
    juce-framework/JUCE
)

# Windows' VST3 folder needs admin, so copying is opt-in there
if(WIN32)
    set(copyDefault OFF)
else()
    set(copyDefault ON)
endif()
option(PLUGIN_COPY_AFTER_BUILD "Copy the plugin into the system plug-in folders after building" ${copyDefault})

juce_add_plugin(${PROJECT_NAME}
        COMPANY_NAME
          SampleRealm
        IS_SYNTH
          TRUE
        NEEDS_MIDI_INPUT
          TRUE
        NEEDS_MIDI_OUTPUT
          FALSE
        IS_MIDI_EFFECT
          FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS
          FALSE
        JUCE_VST3_CAN_REPLACE_VST2
          FALSE
        COPY_PLUGIN_AFTER_BUILD
          ${PLUGIN_COPY_AFTER_BUILD}
        PLUGIN_MANUFACTURER_CODE
          Sprm
        PLUGIN_CODE
          Mdlt
        VERSION 
          0.0.1
        FORMATS
          VST3
          AU
          Standalone
        PRODUCT_NAME "${PROJECT_NAME}"
)

# Automatically find all source files in Source directory
file(GLOB_RECURSE SourceFiles
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

# How we want our SourceFiles to appear in our IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SourceFiles})

# Make the SourceFiles buildable
target_sources(${PROJECT_NAME} PRIVATE ${SourceFiles})

# These are some toggleable options from the JUCE CMake API
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Binary assets (images, fonts, etc.)
# Automatically find all files in Assets directory
file(GLOB_RECURSE AssetFiles
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*"
)

# Only create binary data target if assets exist
if(AssetFiles)
    juce_add_binary_data(${PROJECT_NAME}_Assets
        HEADER_NAME "BinaryData.h"
        NAMESPACE BinaryData
        SOURCES ${AssetFiles}
    )
    set(ASSET_TARGET ${PROJECT_NAME}_Assets)
else()
    message(STATUS "No assets found in Assets/ directory")
    set(ASSET_TARGET "")
endif()

# JUCE libraries to bring into our project
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${ASSET_TARGET}  # Only links if assets exist
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Launches the Standalone the same way on macOS and Windows
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}_Standalone>
    DEPENDS ${PROJECT_NAME}_Standalone
    USES_TERMINAL)

# Windows installs on demand, since its VST3 folder needs an administrator prompt
if(WIN32)
    get_target_property(vst3Artefact ${PROJECT_NAME}_VST3 JUCE_PLUGIN_ARTEFACT_FILE)
    get_target_property(vst3CopyDir  ${PROJECT_NAME}_VST3 JUCE_PLUGIN_COPY_DIR)
    add_custom_target(install_plugin
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${vst3Artefact}"
                "${vst3CopyDir}/$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_PRODUCT_NAME>.vst3"
        DEPENDS ${PROJECT_NAME}_VST3
        COMMENT "Installing VST3 into ${vst3CopyDir}")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Pluginval.cmake)
add_pluginval_tests(${PROJECT_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Tests.cmake)
add_unit_tests(${PROJECT_NAME})