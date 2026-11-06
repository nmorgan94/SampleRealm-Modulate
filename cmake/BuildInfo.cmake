# BuildInfo.h: PLUGIN_GIT_COMMIT is the short hash, with -local for uncommitted changes, or unknown

# Run on every build by add_build_info's target; rewrites the header only when the value changes
if(CMAKE_SCRIPT_MODE_FILE)
    set(commit unknown)

    # Neither command writes the index, so a build can't clash with a commit
    if(GIT)
        execute_process(COMMAND "${GIT}" --no-optional-locks rev-parse --short HEAD
            WORKING_DIRECTORY "${REPO}"
            OUTPUT_VARIABLE hash OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE failed ERROR_QUIET)

        if(NOT failed)
            execute_process(COMMAND "${GIT}" --no-optional-locks diff --quiet HEAD --
                WORKING_DIRECTORY "${REPO}"
                RESULT_VARIABLE changed ERROR_QUIET)

            set(commit "${hash}")
            if(changed EQUAL 1)
                string(APPEND commit "-local")
            endif()
        endif()
    endif()

    file(CONFIGURE OUTPUT "${OUT}"
        CONTENT "#pragma once\n\n#define PLUGIN_GIT_COMMIT \"@commit@\"\n"
        @ONLY)
    return()
endif()

include_guard(GLOBAL)

find_package(Git QUIET)

function(add_build_info target)
    set(header "${CMAKE_CURRENT_BINARY_DIR}/generated/BuildInfo.h")

    add_custom_target(${target}_BuildInfo
        COMMAND "${CMAKE_COMMAND}" "-DGIT=${GIT_EXECUTABLE}" "-DREPO=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.."
                "-DOUT=${header}" -P "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        BYPRODUCTS "${header}"
        VERBATIM)

    add_dependencies(${target} ${target}_BuildInfo)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endfunction()
