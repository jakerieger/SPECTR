cmake_minimum_required(VERSION 3.14)
project(MyPlugin VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20) # Change this if you want, C++17 and up is recommended
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# Add your embedded resources here
# juce_add_binary_data(EmbeddedResources
#     SOURCES
#     # sources...
#     HEADER_NAME BinaryData.h
#     NAMESPACE BinaryData
# )

juce_add_plugin(MyPlugin
    # Basic plugin info
    COMPANY_NAME "My Company"
    BUNDLE_ID "com.MyCompany.MyPlugin"
    PLUGIN_MANUFACTURER_CODE MANU  # 4 characters, change this
    PLUGIN_CODE PLUG               # 4 characters, change this
    FORMATS VST3                   # Only VST3 format
    PRODUCT_NAME "My Plugin"        # The name that will be displayed in DAWs and in the plugin window titlebar. This can contain spaces.

    # Plugin characteristics
    IS_SYNTH FALSE                 # Set to TRUE if it's a synthesizer
    NEEDS_MIDI_INPUT FALSE         # Set to TRUE if plugin needs MIDI
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # Copy plugin to system folders for testing
    COPY_PLUGIN_AFTER_BUILD FALSE
    #    VST3_COPY_DIR "~/Library/Audio/Plug-Ins/VST3/My Company"  # macOS
    VST3_COPY_DIR "C:/Program Files/Common Files/VST3/My Company"  # Windows
)

target_sources(MyPlugin PRIVATE
    Code/PluginProcessor.cpp
    Code/PluginProcessor.hpp
    Code/PluginEditor.cpp
    Code/PluginEditor.hpp
)

# Add any JUCE modules your plugin needs here. Un-comment the EmbeddedResources if you're using it.
target_link_libraries(MyPlugin PRIVATE
    #    EmbeddedResources
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_dsp
)

target_compile_definitions(MyPlugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(MyPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MyPlugin PRIVATE DEBUG=1 _DEBUG=1)
else ()
    target_compile_definitions(MyPlugin PRIVATE NDEBUG=1)
endif ()