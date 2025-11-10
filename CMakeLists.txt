cmake_minimum_required(VERSION 3.22)
project(ROW_MANAGER VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(../JUCE ignore EXCLUDE_FROM_ALL)                    # If you've put JUCE in a subdirectory called JUCE

include_directories(
    libs/choc/choc
)

juce_add_plugin(RowManager
    # VERSION ...                               # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    # COMPANY_NAME ...                          # Specify the name of the plugin's author
    # IS_SYNTH TRUE/FALSE                       # Is this a synth or an effect?
    NEEDS_MIDI_INPUT TRUE               # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT TRUE              # Does the plugin need midi output?
    # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
    COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE Xenk               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE 67ae                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS AU VST3 Standalone                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "Row Manager")        # The name of the final executable, which can differ from the target name

# juce_generate_juce_header(AudioPluginExample)

target_sources(RowManager
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp)

target_compile_options(RowManager PRIVATE -Werror=return-type)
target_compile_definitions(RowManager
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(AudioPluginData SOURCES ...)

target_link_libraries(RowManager
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        # juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

add_executable(TestingProgram 
    Source/testing.cpp 
    )
# Source/Experimental/xaudiograph.cpp
target_compile_definitions(TestingProgram PRIVATE XENPYTHONBINDINGS=0 NOJUCE=1 _USE_MATH_DEFINES=1)
target_compile_options(TestingProgram PRIVATE -Werror=return-type)
