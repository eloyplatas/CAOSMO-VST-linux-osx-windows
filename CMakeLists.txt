cmake_minimum_required(VERSION 3.22)
project(Caosmo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Point JUCE_DIR at your JUCE checkout (default: ../JUCE)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(Caosmo
    COMPANY_NAME "Eloy Platas"
    BUNDLE_ID com.eloyplatas.caosmo
    PLUGIN_MANUFACTURER_CODE Elpl
    PLUGIN_CODE Csmo
    FORMATS VST3 Standalone
    PRODUCT_NAME "Caosmo"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth)

juce_add_binary_data(CaosmoData SOURCES presets/factory_presets.xml docs/manual.html)

# --- Effects from the Synthstrom Audible Deluge firmware (GPL-3.0-or-later), see Source/deluge/README.md ---
set(DELUGE_SOURCES
    Source/DelugeFX.cpp
    Source/deluge/model/mod_controllable/ModFXProcessor.cpp
    Source/deluge/dsp/reverb/freeverb/freeverb.cpp
    Source/deluge/dsp/delay/delay.cpp
    Source/deluge/dsp/delay/delay_buffer.cpp
    Source/deluge/dsp/compressor/rms_feedback.cpp
    Source/deluge/dsp/granular/GranularProcessor.cpp
    Source/deluge/model/fx/stutterer.cpp
    Source/deluge/modulation/lfo.cpp
    Source/deluge/shim/deluge_support.cpp)
# The Deluge code is C++23 with GNU attributes: GCC/Clang get gnu++23 (gnu++2b on older AppleClang), Windows uses clang-cl.
if (MSVC)
    set(DELUGE_CXX_FLAGS "/std:c++latest;-Wno-everything")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    set(DELUGE_CXX_FLAGS "-std=gnu++2b;-w")
else()
    set(DELUGE_CXX_FLAGS "-std=gnu++23;-w")
endif()
set_source_files_properties(${DELUGE_SOURCES} PROPERTIES
    COMPILE_OPTIONS "${DELUGE_CXX_FLAGS}"
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/Source/deluge/shim;${CMAKE_CURRENT_SOURCE_DIR}/Source/deluge;${CMAKE_CURRENT_SOURCE_DIR}/Source")

set(CAOSMO_SOURCES
    Source/Params.cpp
    Source/Shapes.cpp
    Source/PresetManager.cpp
    Source/UI/Widgets.cpp
    Source/UI/Visuals.cpp
    Source/UI/Pages.cpp
    Source/UI/Overlays.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${DELUGE_SOURCES})

target_sources(Caosmo PRIVATE ${CAOSMO_SOURCES})
target_compile_definitions(Caosmo PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Caosmo
    PRIVATE CaosmoData juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

option(CAOSMO_TESTS "Build offline test harness" ON)
if(CAOSMO_TESTS)
  juce_add_console_app(CaosmoTest PRODUCT_NAME "CaosmoTest")
  target_sources(CaosmoTest PRIVATE tests/TestMain.cpp tests/PresetGen.cpp ${CAOSMO_SOURCES})
  target_compile_definitions(CaosmoTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(CaosmoTest PRIVATE CaosmoData juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp juce::juce_recommended_config_flags)
endif()
