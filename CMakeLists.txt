cmake_minimum_required(VERSION 3.22)
project(GlasklarAmp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(GlasklarAmp
    COMPANY_NAME "NWTE"
    PLUGIN_MANUFACTURER_CODE Nwte
    PLUGIN_CODE Glsk
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Glasklar Amp"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(GlasklarAmp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(GlasklarAmp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GlasklarAmp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Optional: disable JUCE GUI warnings while building plugin examples / extras.
# This keeps the project lean and avoids extra example artifacts.
set_property(TARGET GlasklarAmp PROPERTY CXX_STANDARD 17)
