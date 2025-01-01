cmake_minimum_required(VERSION 3.22)
project(SideQick VERSION 0.0.1)

# JUCE setup
add_subdirectory(JUCE)

# Set JUCE configuration options
juce_add_gui_app(SideQick
    PRODUCT_NAME "SideQick"
    COMPANY_NAME "VincentZauhar"
)

# Add source files
target_sources(SideQick
    PRIVATE
        Source/Display.cpp
        Source/Logo.cpp
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MidiSysexProcessor.cpp
        Source/PannelButton.cpp
        Source/ProgramParser.cpp
)

# Add preprocessor definitions
target_compile_definitions(SideQick
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Add binary data
juce_add_binary_data(SideQickBinaryData SOURCES
    Assets/DSEG14Classic-Italic.ttf
    Assets/logo.svg    
    Assets/plastic.png
)

juce_generate_juce_header(SideQick)

# Link libraries
target_link_libraries(SideQick
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        SideQickBinaryData
)