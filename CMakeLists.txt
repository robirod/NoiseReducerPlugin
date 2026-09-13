cmake_minimum_required(VERSION 3.22)
project(NoiseReducerPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fuentes del Motor DSP Core (Independiente) ---
set(DSP_SOURCES
    src/dsp/FFTEngine.cpp
    src/dsp/FFTEngine.h
    src/dsp/STFTBuffer.cpp
    src/dsp/STFTBuffer.h
    src/dsp/NoiseGate.cpp
    src/dsp/NoiseGate.h
    src/dsp/SpectralSub.cpp
    src/dsp/SpectralSub.h
    src/dsp/WienerFilter.cpp
    src/dsp/WienerFilter.h
    src/dsp/NoiseReducerCore.cpp
    src/dsp/NoiseReducerCore.h
)

# --- 1. Librería C++ DSP Core ---
add_library(NoiseReducerDSP STATIC ${DSP_SOURCES})
target_include_directories(NoiseReducerDSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp)

# --- 2. Aplicación CLI de Consola ---
add_executable(noise_reducer_cli src/cli/main_cli.cpp)
target_link_libraries(noise_reducer_cli PRIVATE NoiseReducerDSP)

# --- 3. Suite de Pruebas Unitarias C++ ---
add_executable(noise_reducer_tests tests/dsp_tests.cpp)
target_link_libraries(noise_reducer_tests PRIVATE NoiseReducerDSP)

# --- 4. Integración JUCE Audio Plugin (Opcional si FetchContent disponible) ---
option(BUILD_JUCE_PLUGIN "Compilar el plugin VST3/AU usando JUCE" ON)

if(BUILD_JUCE_PLUGIN)
    include(FetchContent)
    message(STATUS "Configurando JUCE vía FetchContent...")
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)

    juce_add_plugin(NoiseReducerPlugin
        COMPANY_NAME "AudioDSP"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE ADsp
        PLUGIN_CODE NRed
        FORMATS VST3 AU Standalone
        PRODUCT_NAME "Noise Reducer"
    )

    target_sources(NoiseReducerPlugin PRIVATE
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginProcessor.h
        src/plugin/PluginEditor.cpp
        src/plugin/PluginEditor.h
    )

    target_link_libraries(NoiseReducerPlugin PRIVATE
        NoiseReducerDSP
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    )

    juce_generate_juce_header(NoiseReducerPlugin)
endif()
