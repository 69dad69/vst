# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.31.6-msvc6)

# Reject any attempt to use a toolchain file. We must not use one because
# we could be downloading it here. If the CMAKE_TOOLCHAIN_FILE environment
# variable is set, the cache variable will have been initialized from it.
unset(CMAKE_TOOLCHAIN_FILE CACHE)
unset(ENV{CMAKE_TOOLCHAIN_FILE})

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(juce-populate NONE)


# Pass through things we've already detected in the main project to avoid
# paying the cost of redetecting them again in ExternalProject_Add()
set(GIT_EXECUTABLE [==[GIT_EXECUTABLE-NOTFOUND]==])
set(GIT_VERSION_STRING [==[]==])
set_property(GLOBAL PROPERTY _CMAKE_FindGit_GIT_EXECUTABLE_VERSION
  [==[GIT_EXECUTABLE-NOTFOUND;]==]
)


include(ExternalProject)
ExternalProject_Add(juce-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/juce-framework/JUCE.git" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "GIT_TAG" "e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8" "GIT_PROGRESS" "TRUE"
                    SOURCE_DIR          "C:/Users/DAD/Downloads/VoiceDub_VST3_0.2_ready_to_compile (1)/build/vst3-release/_deps/juce-src"
                    BINARY_DIR          "C:/Users/DAD/Downloads/VoiceDub_VST3_0.2_ready_to_compile (1)/build/vst3-release/_deps/juce-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


