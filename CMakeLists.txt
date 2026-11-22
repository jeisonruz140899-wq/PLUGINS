cmake_minimum_required(VERSION 3.22)
project(SilkSat VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# ---------------- Plugin (VST3) ----------------
juce_add_plugin(SilkSat
    COMPANY_NAME "DTH"
    PLUGIN_MANUFACTURER_CODE Dthp
    PLUGIN_CODE Slks
    FORMATS VST3
    PRODUCT_NAME "SilkSat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(SilkSat PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(SilkSat PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SilkSat
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# ---------------- Prueba offline de DSP (consola) ----------------
juce_add_console_app(SilkTest PRODUCT_NAME "SilkTest")
target_sources(SilkTest PRIVATE
    tests/Test.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_compile_definitions(SilkTest PRIVATE
    SILKSAT_TEST_BUILD=1
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SilkTest
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)

# ---------------- SilkHeat (VST3) ----------------
juce_add_plugin(SilkHeat
    COMPANY_NAME "DTH"
    PLUGIN_MANUFACTURER_CODE Dthp
    PLUGIN_CODE Slkh
    FORMATS VST3
    PRODUCT_NAME "SilkHeat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(SilkHeat PRIVATE Source/Heat/HeatProcessor.cpp Source/Heat/HeatEditor.cpp)
target_compile_definitions(SilkHeat PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SilkHeat
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_console_app(SilkHeatTest PRODUCT_NAME "SilkHeatTest")
target_sources(SilkHeatTest PRIVATE tests/TestHeat.cpp Source/Heat/HeatProcessor.cpp Source/Heat/HeatEditor.cpp)
target_compile_definitions(SilkHeatTest PRIVATE SILKSAT_TEST_BUILD=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SilkHeatTest PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)

# Captura de las interfaces (revisión visual en Linux)
juce_add_console_app(SilkShot PRODUCT_NAME "SilkShot")
target_sources(SilkShot PRIVATE tests/Shot.cpp
    Source/PluginProcessor.cpp Source/PluginEditor.cpp
    Source/Heat/HeatProcessor.cpp Source/Heat/HeatEditor.cpp)
target_compile_definitions(SilkShot PRIVATE SILKSAT_TEST_BUILD=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SilkShot PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
