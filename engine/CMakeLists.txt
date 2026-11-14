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
    src/instruments/drum_kit.cpp
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
    src/audio_exporter.cpp
    src/project_serializer.cpp
)

# JUCE modules are INTERFACE libraries that carry their own sources. Linking
# them PRIVATE compiles JUCE exactly once, into this static library; linking
# them PUBLIC would recompile all of JUCE into every target that uses the
# engine (the tests and the Node addon). The INTERFACE generator expressions
# below still hand JUCE's include paths and definitions on to those targets.
# This is the arrangement recommended in JUCE's "CMake API" documentation.
target_link_libraries(calliope_engine
    PRIVATE
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
    INTERFACE
        $<TARGET_PROPERTY:calliope_engine,COMPILE_DEFINITIONS>
)

target_include_directories(calliope_engine
    PUBLIC
        include
    INTERFACE
        $<TARGET_PROPERTY:calliope_engine,INCLUDE_DIRECTORIES>
)

if(MSVC)
    # Compile translation units in parallel.
    target_compile_options(calliope_engine PRIVATE /MP)
endif()
