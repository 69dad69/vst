cmake_minimum_required(VERSION 3.24)

project(VoiceDub VERSION 0.2.0 LANGUAGES C CXX)

option(VOICEDUB_BUILD_PLUGIN "Build the VST3 plugin" ON)
option(VOICEDUB_BUILD_TESTS "Build standalone core tests" ON)
set(VOICEDUB_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")

add_library(VoiceDubCore STATIC
    Source/VoiceDubCore.cpp
    Source/VoiceDubCore.h
)

target_include_directories(VoiceDubCore PUBLIC Source)
target_compile_features(VoiceDubCore PUBLIC cxx_std_20)
set_target_properties(VoiceDubCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (MSVC)
    target_compile_options(VoiceDubCore PRIVATE /W4 /permissive-)
else()
    target_compile_options(VoiceDubCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (VOICEDUB_BUILD_TESTS)
    enable_testing()
    add_executable(VoiceDubCoreTests Tests/CoreTests.cpp)
    target_link_libraries(VoiceDubCoreTests PRIVATE VoiceDubCore)
    target_compile_features(VoiceDubCoreTests PRIVATE cxx_std_20)
    add_test(NAME VoiceDubCoreTests COMMAND VoiceDubCoreTests)
endif()

if (VOICEDUB_BUILD_PLUGIN)
    if (NOT VOICEDUB_JUCE_PATH AND DEFINED ENV{JUCE_PATH})
        set(VOICEDUB_JUCE_PATH "$ENV{JUCE_PATH}")
    endif()

    if (VOICEDUB_JUCE_PATH)
        if (NOT EXISTS "${VOICEDUB_JUCE_PATH}/CMakeLists.txt")
            message(FATAL_ERROR "VOICEDUB_JUCE_PATH does not point to a JUCE source tree: ${VOICEDUB_JUCE_PATH}")
        endif()
        add_subdirectory("${VOICEDUB_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
    else()
        include(FetchContent)
        set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(JUCE)
    endif()

    juce_add_plugin(VoiceDub
        COMPANY_NAME "OpenAudioLab"
        BUNDLE_ID "audio.openaudiolab.voicedub"
        DESCRIPTION "Monophonic voice-to-MIDI pitch tracking plugin"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE OALb
        PLUGIN_CODE Vd01
        FORMATS VST3
        VST3_CATEGORIES Fx Analyzer Tools
        PRODUCT_NAME "VoiceDub"
    )

    juce_generate_juce_header(VoiceDub)

    target_sources(VoiceDub PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
    )

    target_compile_features(VoiceDub PRIVATE cxx_std_20)

    target_compile_definitions(VoiceDub PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(VoiceDub PRIVATE
        VoiceDubCore
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )
endif()
