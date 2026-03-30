add_library(calliope_engine STATIC
    src/engine.cpp
    src/test_tone.cpp
    src/transport.cpp
    src/audio_graph.cpp
    src/master_bus.cpp
    src/metronome.cpp
    src/command_dispatcher.cpp
    src/parameter_registry.cpp
    src/project_state.cpp
    src/commands/transport_commands.cpp
    src/commands/parameter_commands.cpp
    src/commands/instrument_commands.cpp
    src/commands/effect_commands.cpp
    src/instruments/drum_machine.cpp
    src/instruments/poly_synth.cpp
    src/instruments/poly_synth_voice.cpp
    src/instruments/bass_synth.cpp
    src/instruments/bass_synth_voice.cpp
    src/effects/parametric_eq.cpp
    src/effects/compressor.cpp
    src/effects/reverb.cpp
    src/effects/delay.cpp
    src/effects/limiter.cpp
    src/insert_chain.cpp
    src/insert_chain_processor.cpp
    src/clip_scheduler.cpp
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
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_data_structures
)

target_compile_definitions(calliope_engine
    PUBLIC
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
