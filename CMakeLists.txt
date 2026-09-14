cmake_minimum_required(VERSION 3.22)

project(TapeDelayEQ VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- JUCE ---------------------------------------------------------------
# Si prefieres usar JUCE como submódulo en vez de descargarlo en cada build,
# reemplaza este bloque por: add_subdirectory(JUCE)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(TapeDelayEQ
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Yrcp
    PLUGIN_CODE Tdeq
    FORMATS VST3 Standalone
    PRODUCT_NAME "Tape Delay EQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
)

target_sources(TapeDelayEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(TapeDelayEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(TapeDelayEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
