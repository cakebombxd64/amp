cmake_minimum_required(VERSION 3.22)
project(GlasklarAmp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE (Version 8) muss als Ordner "JUCE" neben dieser Datei liegen (siehe README.md)
add_subdirectory(JUCE)

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
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(GlasklarAmp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(GlasklarAmp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GlasklarAmp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
