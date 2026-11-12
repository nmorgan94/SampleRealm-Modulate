# CPM, and SampleRealm-Modules as webui/package.json names it

include_guard(GLOBAL)

set(CPM_DOWNLOAD_VERSION 0.43.1)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

# CPM's own override would move the C++ side only
if(DEFINED CPM_SampleRealm-Modules_SOURCE)
    message(FATAL_ERROR "Reconfigure with -UCPM_SampleRealm-Modules_SOURCE: webui/package.json names the copy to use")
endif()

set(webuiDir "${CMAKE_CURRENT_LIST_DIR}/../webui")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${webuiDir}/package.json")

file(READ "${webuiDir}/package.json" packageJson)
string(JSON modulesPackage GET "${packageJson}" dependencies "@samplerealm/modules")

if(modulesPackage MATCHES "^file:(.+)$")
    cmake_path(ABSOLUTE_PATH CMAKE_MATCH_1 BASE_DIRECTORY "${webuiDir}" NORMALIZE OUTPUT_VARIABLE modulesDir)
    if(NOT IS_DIRECTORY "${modulesDir}")
        message(FATAL_ERROR "webui/package.json names @samplerealm/modules as a local folder that isn't here: ${modulesDir}")
    endif()
    message(STATUS "SampleRealm-Modules: the local folder ${modulesDir}")
    set(SampleRealm-Modules_SOURCE_DIR "${modulesDir}")
elseif(modulesPackage MATCHES "^github:([^#]+)#([0-9a-f]+)$")
    string(LENGTH "${CMAKE_MATCH_2}" commitLength)
    if(NOT commitLength EQUAL 40)
        message(FATAL_ERROR "webui/package.json must name @samplerealm/modules by a full commit id, not: ${modulesPackage}")
    endif()
    CPMAddPackage(NAME SampleRealm-Modules GITHUB_REPOSITORY "${CMAKE_MATCH_1}" GIT_TAG "${CMAKE_MATCH_2}" DOWNLOAD_ONLY YES)
else()
    message(FATAL_ERROR "webui/package.json must name @samplerealm/modules as github:<owner>/<repo>#<commit id> or file:<folder>, not: ${modulesPackage}")
endif()
