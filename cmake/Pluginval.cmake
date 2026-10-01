# Builds Tracktion's pluginval from source and registers it as CTest tests.
# pluginval loads your built plugin the way a host would and runs a graded set of
# stability tests: bus layouts, state save/restore, editor open/close, parameter
# access off the message thread, and more. It exits 0 on pass and 1 on failure,
# which is exactly CTest's contract, so wiring it up is a thin wrapper.
#
# Usage (from the top-level CMakeLists.txt, AFTER juce_add_plugin):
#     include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Pluginval.cmake)
#     add_pluginval_tests(${PROJECT_NAME})

include_guard(GLOBAL)

# Set to OFF to skip building pluginval entirely (e.g. for a fast CI compile-only job)
option(ENABLE_PLUGINVAL "Build pluginval and register validation tests" ON)

set(PLUGINVAL_STRICTNESS 10 CACHE STRING "pluginval strictness level, 1 to 10")
set_property(CACHE PLUGINVAL_STRICTNESS PROPERTY STRINGS 1 2 3 4 5 6 7 8 9 10)

set(PLUGINVAL_SAMPLERATES "44100;48000;96000" CACHE STRING "Sample rates to test at")
set(PLUGINVAL_BLOCKSIZES  "64;512;1024"       CACHE STRING "Block sizes to test at")
set(PLUGINVAL_REPEATS 0 CACHE STRING "Times to repeat tests without reinstantiating the plugin")
set(PLUGINVAL_TIMEOUT 1800 CACHE STRING "Seconds before CTest kills a validation run")

# Steinberg's conformance validator, embedded into pluginval. Upstream defaults this ON,
# which drags in the whole VST3 SDK (~300MB of submodules) for one extra test. We declare
# it ourselves so the default is OFF but -DPLUGINVAL_VST3_VALIDATOR=ON is still honoured
option(PLUGINVAL_VST3_VALIDATOR "Build pluginval with Steinberg's embedded VST3 validator" OFF)

if(ENABLE_PLUGINVAL)
    CPMAddPackage(
        NAME
          pluginval
        GIT_TAG
          4c5adc2c1a9910251667152166139a0c37b953e6
        GITHUB_REPOSITORY
          Tracktion/pluginval
        EXCLUDE_FROM_ALL
          YES
        SYSTEM
          YES
        OPTIONS
          "PLUGINVAL_VST3_VALIDATOR ${PLUGINVAL_VST3_VALIDATOR}"
    )

    if(TARGET pluginval)
        set_target_properties(pluginval PROPERTIES
            FOLDER "Tools"
            OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
else()
    message(STATUS "pluginval: disabled (ENABLE_PLUGINVAL=OFF)")
endif()

function(add_pluginval_tests pluginTarget)
    if(NOT TARGET pluginval)
        if(ENABLE_PLUGINVAL)
            message(WARNING "add_pluginval_tests: pluginval target not available, skipping")
        endif()

        return()
    endif()

    list(JOIN PLUGINVAL_SAMPLERATES "," sampleRates)
    list(JOIN PLUGINVAL_BLOCKSIZES  "," blockSizes)

    get_target_property(formatTargets ${pluginTarget} JUCE_ACTIVE_PLUGIN_TARGETS)

    set(builtTargets "")

    foreach(formatTarget IN LISTS formatTargets)
        get_target_property(format ${formatTarget} JUCE_TARGET_KIND_STRING)


        if(format STREQUAL "App")
            continue()
        endif()

        if(format STREQUAL "AU")
            get_target_property(copyDir ${formatTarget} JUCE_PLUGIN_COPY_DIR)

            if(NOT copyDir)
                message(WARNING "pluginval: AU test needs COPY_PLUGIN_AFTER_BUILD, skipping that test")
                continue()
            endif()

            set(artefact "${copyDir}/$<TARGET_BUNDLE_DIR_NAME:${formatTarget}>")
        else()
            get_target_property(artefact ${formatTarget} JUCE_PLUGIN_ARTEFACT_FILE)
        endif()

        set(testName "${pluginTarget}.pluginval.${format}")

        add_test(NAME "${testName}"
                 COMMAND $<TARGET_FILE:pluginval>
                         --strictness-level "${PLUGINVAL_STRICTNESS}"
                         --sample-rates "${sampleRates}"
                         --block-sizes "${blockSizes}"
                         --repeat "${PLUGINVAL_REPEATS}"
                         --randomise
                         --output-dir "${CMAKE_BINARY_DIR}/pluginval-logs"
                         --validate "${artefact}")

        set_tests_properties("${testName}" PROPERTIES TIMEOUT "${PLUGINVAL_TIMEOUT}")

        list(APPEND builtTargets ${formatTarget})
        message(STATUS "pluginval: registered ${testName}")
    endforeach()

    if(NOT builtTargets)
        message(WARNING "pluginval: no validatable formats for ${pluginTarget}, validate will fail")
    endif()

    # --no-tests=error so an empty or mistyped filter fails instead of exiting 0
    add_custom_target(${pluginTarget}_validate
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --no-tests=error --parallel 0
                                       -R "${pluginTarget}\\.pluginval"
        DEPENDS pluginval ${builtTargets}
        COMMENT "Validating ${pluginTarget} with pluginval")

    if(NOT TARGET validate)
        add_custom_target(validate)
    endif()

    add_dependencies(validate ${pluginTarget}_validate)
endfunction()
