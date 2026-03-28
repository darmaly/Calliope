#include <catch2/catch_test_macros.hpp>
#include "calliope/audio_graph.h"

using namespace calliope;

TEST_CASE("AudioGraph initialises with default settings", "[AudioGraph][integration]")
{
    AudioGraph graph;
    REQUIRE(graph.initialise(44100.0, 512));
    REQUIRE(graph.isInitialised());
    graph.shutdown();
}

TEST_CASE("AudioGraph initialises with 48kHz", "[AudioGraph][integration]")
{
    AudioGraph graph;
    REQUIRE(graph.initialise(48000.0, 512));
    REQUIRE(graph.getAudioConfig().sampleRate == 48000.0);
    graph.shutdown();
}

TEST_CASE("AudioGraph accepts buffer sizes 128-2048", "[AudioGraph][integration]")
{
    AudioGraph graph;

    for (int bufSize : {128, 256, 512, 1024, 2048}) {
        SECTION("Buffer size " + std::to_string(bufSize)) {
            REQUIRE(graph.initialise(44100.0, bufSize));
            REQUIRE(graph.getAudioConfig().bufferSize == bufSize);
            graph.shutdown();
        }
    }
}

TEST_CASE("AudioGraph provides Transport reference", "[AudioGraph][integration]")
{
    AudioGraph graph;
    REQUIRE(graph.initialise(44100.0, 512));
    REQUIRE(graph.getTransport().getBpm() == 120.0);
    graph.shutdown();
}

TEST_CASE("AudioGraph provides MasterBus reference", "[AudioGraph][integration]")
{
    AudioGraph graph;
    REQUIRE(graph.initialise(44100.0, 512));
    REQUIRE(graph.getMasterBus().masterVolume.load() == 1.0f);
    graph.shutdown();
}

TEST_CASE("AudioGraph provides Metronome reference", "[AudioGraph][integration]")
{
    AudioGraph graph;
    REQUIRE(graph.initialise(44100.0, 512));
    REQUIRE(graph.getMetronome().enabled.load() == true);
    graph.shutdown();
}

TEST_CASE("AudioGraph setBufferSize reconfigures", "[AudioGraph][integration]")
{
    AudioGraph graph;
    REQUIRE(graph.initialise(44100.0, 512));
    REQUIRE(graph.setBufferSize(1024));
    REQUIRE(graph.getAudioConfig().bufferSize == 1024);
    graph.shutdown();
}
