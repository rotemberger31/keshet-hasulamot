cmake_minimum_required(VERSION 3.22)

# Universal binary, so it loads whether Live runs natively or under Rosetta.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(Keshet VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(deps/JUCE)

set(KESHET_SOURCES
    Source/music/Music.cpp
    Source/dsp/Synth.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

juce_add_plugin(Keshet
    COMPANY_NAME "Rotem Berger"
    BUNDLE_ID com.rotemberger.keshet
    PLUGIN_MANUFACTURER_CODE Rtbg
    PLUGIN_CODE Ksht
    PRODUCT_NAME "Keshet"
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(Keshet PRIVATE ${KESHET_SOURCES})
target_compile_definitions(Keshet PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Keshet
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

# Pure music logic: the matrix of src/music/music.test.ts.
add_executable(MusicTests tests/MusicTests.cpp Source/music/Music.cpp)

# Drives the processor with a fake host and renders the editor to a PNG.
juce_add_console_app(RenderTests PRODUCT_NAME "RenderTests")
target_sources(RenderTests PRIVATE tests/RenderTests.cpp ${KESHET_SOURCES})
target_compile_definitions(RenderTests PRIVATE
    JucePlugin_Name="Keshet"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(RenderTests
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags)
