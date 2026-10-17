cmake_minimum_required(VERSION 3.22)
project(AtelierInstruments VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
endif()

option(ATELIER_BUILD_PLUGINS "Build the VST3/AU plugins" ON)
option(ATELIER_BUILD_TESTS "Build the offline render test tool" ON)

include(FetchContent)
# Use -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE to build offline.
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(ATELIER_ENGINE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Source/SamplerEngine.cpp)

# ---------------------------------------------------------------------------
function(atelier_instrument target id product code folder sfz)
  juce_add_plugin(${target}
    COMPANY_NAME "Atelier Instruments"
    COMPANY_WEBSITE "https://github.com/salimadour31-boop/atelier-instruments"
    BUNDLE_ID "org.atelier.${target}"
    PLUGIN_MANUFACTURER_CODE Atln
    PLUGIN_CODE ${code}
    PRODUCT_NAME "${product}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Sampler
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS ${ATELIER_FORMATS})
  target_sources(${target} PRIVATE
    Source/PluginProcessor.cpp Source/PluginEditor.cpp ${ATELIER_ENGINE_SOURCES})
  target_compile_definitions(${target} PUBLIC
    ATELIER_INSTRUMENT=${id}
    ATELIER_SFZ_FOLDER="${folder}"
    ATELIER_SFZ_FILE="${sfz}"
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
  target_link_libraries(${target} PRIVATE
    juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

  # copy the samples into each bundle: <bundle>/Contents/Resources/<folder>
  foreach(fmt ${ATELIER_FORMATS})
    if(TARGET ${target}_${fmt})
      get_target_property(art ${target}_${fmt} JUCE_PLUGIN_ARTEFACT_FILE)
      add_custom_command(TARGET ${target}_${fmt} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Instruments/${folder}" "${art}/Contents/Resources/${folder}"
        VERBATIM)
    endif()
  endforeach()
endfunction()

if(ATELIER_BUILD_PLUGINS)
  if(APPLE)
    set(ATELIER_FORMATS VST3 AU)
  else()
    set(ATELIER_FORMATS VST3)
  endif()
  atelier_instrument(AtelierPiano    0 "Atelier Piano"    AtPn Piano    piano.sfz)
  atelier_instrument(AtelierBatterie 1 "Atelier Batterie" AtDr Batterie batterie.sfz)
  atelier_instrument(AtelierCordes   2 "Atelier Cordes"   AtCo Cordes   cordes.sfz)
endif()

if(ATELIER_BUILD_TESTS)
  juce_add_console_app(atelier_render PRODUCT_NAME "atelier_render")
  target_sources(atelier_render PRIVATE tests/render.cpp ${ATELIER_ENGINE_SOURCES})
  target_compile_definitions(atelier_render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(atelier_render PRIVATE juce::juce_audio_formats juce::juce_audio_basics juce::juce_recommended_config_flags)
endif()
