add_library(calliope_engine STATIC
    src/engine.cpp
    src/test_tone.cpp
)

target_include_directories(calliope_engine PUBLIC include)

target_link_libraries(calliope_engine
    PUBLIC
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_events
        juce::juce_dsp
)

target_compile_definitions(calliope_engine
    PUBLIC
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
