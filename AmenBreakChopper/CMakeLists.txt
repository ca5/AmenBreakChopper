cmake_minimum_required(VERSION 3.15)

project(AmenBreakChopper VERSION 1.0.0)

add_subdirectory(../JUCE JUCE)

juce_add_plugin(AmenBreakChopper
    COMPANY_NAME "MyCompany"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Dem0
    FORMATS Standalone VST3 AUv3
    PRODUCT_NAME "Amen Break Chopper")

juce_generate_juce_header(AmenBreakChopper
    NAME AmenBreakChopperAudioProcessor
    VERSION 1.0.0
    COMPANY_NAME "MyCompany"
    WEBSITE "https://juce.com"
    EMAIL "info@juce.com")

target_sources(AmenBreakChopper
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h)

target_compile_definitions(AmenBreakChopper
    PUBLIC
        JUCE_WEB_BROWSER_WORKAROUND=1
        JUCE_USE_CURL=0
        JUCE_DST=1)

target_link_libraries(AmenBreakChopper
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_gui_extra
        juce::juce_dsp
        juce::juce_osc
        juce::juce_opengl)
