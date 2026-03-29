#include <catch2/catch_test_macros.hpp>
#include "calliope/audio_exporter.h"
#include "calliope/engine.h"

TEST_CASE("AudioExporter parseMidiEventsJson", "[export]") {
    auto json = R"([{"beatPosition":0.0,"noteNumber":60,"velocity":100,"durationBeats":1.0,"trackId":"polySynth"}])";
    auto events = calliope::AudioExporter::parseMidiEventsJson(json);
    REQUIRE(events.size() == 1);
    CHECK(events[0].beatPosition == 0.0);
    CHECK(events[0].noteNumber == 60);
    CHECK(events[0].velocity == 100);
    CHECK(events[0].durationBeats == 1.0);
    CHECK(events[0].trackId == "polySynth");
}

TEST_CASE("AudioExporter parseMidiEventsJson empty", "[export]") {
    auto events = calliope::AudioExporter::parseMidiEventsJson("[]");
    REQUIRE(events.empty());
}

TEST_CASE("AudioExporter parseMidiEventsJson multiple events", "[export]") {
    auto json = R"([
        {"beatPosition":0.0,"noteNumber":60,"velocity":100,"durationBeats":1.0,"trackId":"polySynth"},
        {"beatPosition":1.0,"noteNumber":64,"velocity":80,"durationBeats":0.5,"trackId":"bassSynth"},
        {"beatPosition":2.0,"noteNumber":36,"velocity":127,"durationBeats":0.25,"trackId":"drumMachine"}
    ])";
    auto events = calliope::AudioExporter::parseMidiEventsJson(json);
    REQUIRE(events.size() == 3);

    CHECK(events[0].noteNumber == 60);
    CHECK(events[0].trackId == "polySynth");

    CHECK(events[1].noteNumber == 64);
    CHECK(events[1].velocity == 80);
    CHECK(events[1].trackId == "bassSynth");

    CHECK(events[2].noteNumber == 36);
    CHECK(events[2].durationBeats == 0.25);
    CHECK(events[2].trackId == "drumMachine");
}

TEST_CASE("AudioExporter parseMidiEventsJson invalid input", "[export]") {
    auto events = calliope::AudioExporter::parseMidiEventsJson("not json");
    REQUIRE(events.empty());

    auto events2 = calliope::AudioExporter::parseMidiEventsJson("");
    REQUIRE(events2.empty());
}

TEST_CASE("AudioExporter exportMix creates WAV file", "[export][integration]") {
    auto& engine = calliope::Engine::getInstance();
    engine.initialise(44100.0, 512);
    calliope::AudioExporter exporter(engine);

    calliope::ExportOptions opts;
    opts.outputPath = "/tmp/calliope_test_export.wav";
    opts.format = calliope::ExportFormat::WAV_16;
    opts.totalBeats = 4.0;

    bool progressCalled = false;
    bool result = exporter.exportMix(opts, [&progressCalled](float pct) {
        progressCalled = true;
        CHECK(pct >= 0.0f);
        CHECK(pct <= 1.0f);
    });

    CHECK(result == true);
    CHECK(juce::File(opts.outputPath).existsAsFile());
    CHECK(progressCalled == true);

    // Cleanup
    juce::File(opts.outputPath).deleteFile();
    engine.shutdown();
}
