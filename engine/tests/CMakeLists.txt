include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(calliope_engine_tests
    test_main.cpp
    test_transport.cpp
    test_lock_free_queue.cpp
    test_audio_graph.cpp
    test_metronome.cpp
    test_command_dispatcher.cpp
    test_parameter_registry.cpp
    test_project_state.cpp
    test_transport_commands.cpp
    test_parameter_commands.cpp
    test_drum_machine.cpp
    test_poly_synth.cpp
    test_bass_synth.cpp
    test_instrument_commands.cpp
    test_effects.cpp
    test_insert_chain.cpp
)

target_link_libraries(calliope_engine_tests
    PRIVATE
        calliope_engine
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(calliope_engine_tests)
