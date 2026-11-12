cmake_minimum_required(VERSION 3.25)

if(NOT CMAKE_TOOLCHAIN_FILE)
    execute_process(
        COMMAND em-config EMSCRIPTEN_ROOT
        OUTPUT_VARIABLE emscriptenRoot
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    set(CMAKE_TOOLCHAIN_FILE "${emscriptenRoot}/cmake/Modules/Platform/Emscripten.cmake")
endif()

project(WebDemo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules.cmake)
include(${SampleRealm-Modules_SOURCE_DIR}/cmake/WasmDemo.cmake)

# A new .cpp in Source/dsp/ goes here by hand
add_wasm_demo(
    SOURCES
        Engine.cpp
        ../Source/dsp/ModulateSynth.cpp
        ../Source/dsp/WavetableVoice.cpp)

# The filter is JUCE's
target_link_libraries(Engine PRIVATE juce::juce_dsp)

# Size over speed, for the demo's download; the link step optimises too
target_compile_options(Engine PRIVATE $<$<CONFIG:Release>:-Oz>)
target_link_options(Engine PRIVATE $<$<CONFIG:Release>:-Oz>)
