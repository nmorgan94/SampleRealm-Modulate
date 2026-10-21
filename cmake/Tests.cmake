# Builds a Catch2 unit test executable that links the plugin's shared code and registers
# each test case with CTest. The tests drive the AudioProcessor directly, the way a host
# would, so they check what the plugin does to audio and state rather than just that it loads.
#
# Usage (from the top-level CMakeLists.txt, AFTER juce_add_plugin and add_pluginval_tests):
#     include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Tests.cmake)
#     add_unit_tests(${PROJECT_NAME})

include_guard(GLOBAL)

# Set to OFF to skip fetching Catch2 and registering the unit tests
option(ENABLE_UNIT_TESTS "Build Catch2 unit tests and register them with CTest" ON)

if(ENABLE_UNIT_TESTS)
    CPMAddPackage(
        NAME
          Catch2
        GIT_TAG
          v3.16.0
        GITHUB_REPOSITORY
          catchorg/Catch2
        EXCLUDE_FROM_ALL
          YES
        SYSTEM
          YES
    )

    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
    include(Catch)
else()
    message(STATUS "unit tests: disabled (ENABLE_UNIT_TESTS=OFF)")
endif()

function(add_unit_tests pluginTarget)
    # Global targets for the build presets, existing even when disabled (see Pluginval.cmake)
    foreach(aggregate IN ITEMS unit_tests all_tests)
        if(NOT TARGET ${aggregate})
            add_custom_target(${aggregate})
        endif()
    endforeach()

    if(NOT ENABLE_UNIT_TESTS)
        return()
    endif()

    set(testTarget "${pluginTarget}_Tests")

    file(GLOB_RECURSE TestFiles
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.h"
    )

    # Like pluginval, the tests are opt-in (built by `unit_tests` or `all_tests`), so a plain
    # build only produces the plugin
    add_executable(${testTarget} EXCLUDE_FROM_ALL ${TestFiles})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TestFiles})
    set_target_properties(${testTarget} PROPERTIES FOLDER "Tests")

    # The plugin's shared code target already compiles the JUCE modules, so borrow its include
    # paths instead of linking the modules again (which would compile JUCE a second time and
    # duplicate its symbols). Linking it passes on the module and JucePlugin_* definitions.
    # This mirrors how JUCE links its own format wrappers (see _juce_link_plugin_wrapper)
    target_include_directories(${testTarget} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        $<TARGET_PROPERTY:${pluginTarget},INCLUDE_DIRECTORIES>)

    target_link_libraries(${testTarget} PRIVATE
        ${pluginTarget}
        Catch2::Catch2WithMain)

    catch_discover_tests(${testTarget} TEST_PREFIX "${pluginTarget}.unit.")

    add_dependencies(unit_tests ${testTarget})
    add_dependencies(all_tests ${testTarget})
endfunction()
