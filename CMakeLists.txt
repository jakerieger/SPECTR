cmake_minimum_required(VERSION 3.14)
project(SPECTR VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20) # Change this if you want, C++17 and up is recommended
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------
# mbedTLS — fetched at configure time, statically linked
# -----------------------------------------------------------------------
include(FetchContent)

set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)     # don't build mbedTLS tests
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)    # don't build mbedTLS CLI tools
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        mbedtls
        GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
        GIT_TAG v3.6.2
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(mbedtls)

# -----------------------------------------------------------------------
# JUCE
# -----------------------------------------------------------------------
add_subdirectory(JUCE)

juce_add_binary_data(EmbeddedResources
        SOURCES
        Resources/dropdown.js
        Resources/index.html
        Resources/knob.js
        Resources/main.js
        Resources/pluginSettings.js
        Resources/public_key.pem
        Resources/style.css
        Resources/wtDisplay.js
        Resources/keyboard.js
        Resources/nodeGraphEditor.js

        HEADER_NAME BinaryData.h
        NAMESPACE BinaryData
)

juce_add_plugin(SPECTR
        COMPANY_NAME "Rieger DSP"
        BUNDLE_ID "com.RiegerDSP.SPECTR"
        PLUGIN_MANUFACTURER_CODE RDSP  # 4 characters, change this
        PLUGIN_CODE SPCT               # 4 characters, change this
        FORMATS VST3 Standalone        # Supports VST3 and Standalone executable
        PRODUCT_NAME "SPECTR"          # The name that will be displayed in DAWs and in the plugin window titlebar. This can contain spaces.

        IS_SYNTH TRUE                 # Set to TRUE if it's a synthesizer
        NEEDS_MIDI_INPUT TRUE         # Set to TRUE if plugin needs MIDI
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE

        COPY_PLUGIN_AFTER_BUILD TRUE
        VST3_COPY_DIR "C:/Program Files/Common Files/VST3/Rieger DSP"  # Windows
        STANDALONE_COPY_DIR "C:/Program Files/Rieger DSP"

        NEEDS_WEBVIEW2 TRUE
)

target_sources(SPECTR PRIVATE
        Code/AnalogOscillator.hpp
        Code/LicenseVerifier.cpp
        Code/LicenseVerifier.hpp
        Code/PluginCommon.hpp
        Code/PluginEditor.cpp
        Code/PluginEditor.hpp
        Code/PluginProcessor.cpp
        Code/PluginProcessor.hpp
        Code/Voice.hpp
        Code/Wavetable.hpp
        Code/WavetableBuilder.cpp
        Code/WavetableBuilder.hpp
        Code/WavetableData.hpp
        Code/WavetableOscillator.hpp
)

# Add any JUCE modules your plugin needs here. Un-comment the EmbeddedResources if you're using it.
target_link_libraries(SPECTR PRIVATE
        EmbeddedResources
        MbedTLS::mbedcrypto # crypto + PK, no TLS needed
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

target_compile_definitions(SPECTR PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(SPECTR PRIVATE
        ${CMAKE_SOURCE_DIR}/Code
        ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(SPECTR PRIVATE DEBUG=1 _DEBUG=1)
else ()
    target_compile_definitions(SPECTR PRIVATE NDEBUG=1)
endif ()