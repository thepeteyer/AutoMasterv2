cmake_minimum_required(VERSION 3.22)

project(AutoMaster VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE acquisition (same pattern as the NeoSat sibling project): FetchContent
# by default, or pass -DAUTOMASTER_JUCE_PATH=/path/to/JUCE for a local copy.
# ---------------------------------------------------------------------------
set(AUTOMASTER_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(AUTOMASTER_JUCE_PATH)
    message(STATUS "AutoMaster: using local JUCE at ${AUTOMASTER_JUCE_PATH}")
    add_subdirectory(${AUTOMASTER_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    message(STATUS "AutoMaster: fetching JUCE 8.0.4 via FetchContent")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(AutoMaster
    COMPANY_NAME              "Taberia"
    BUNDLE_ID                 "ca.taberia.automaster"
    PLUGIN_MANUFACTURER_CODE  Tbra
    PLUGIN_CODE               Amst
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "AutoMaster"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Fx" "Mastering"
)

juce_generate_juce_header(AutoMaster)

target_sources(AutoMaster PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Params.cpp
    Source/analysis/AnalysisEngine.cpp
    Source/rules/RulesEngine.cpp
    Source/dsp/MasteringChain.cpp
    Source/gui/TransportBar.cpp
    Source/gui/Meters.cpp
    Source/gui/StageTabs.cpp
    Source/gui/TabbedChainEditor.cpp
)

target_include_directories(AutoMaster PRIVATE
    Source
)

target_compile_definitions(AutoMaster PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(AutoMaster PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
