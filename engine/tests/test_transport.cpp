#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/transport.h"

using namespace calliope;
using Catch::Matchers::WithinAbs;

TEST_CASE("Transport starts in Stopped state", "[Transport]")
{
    Transport t;
    REQUIRE(t.getState() == TransportState::Stopped);
}

TEST_CASE("Transport play/stop/pause transitions", "[Transport]")
{
    Transport t;

    t.play();
    REQUIRE(t.getState() == TransportState::Playing);

    t.pause();
    REQUIRE(t.getState() == TransportState::Paused);

    t.play();
    REQUIRE(t.getState() == TransportState::Playing);

    t.stop();
    REQUIRE(t.getState() == TransportState::Stopped);
}

TEST_CASE("Transport stop resets position to 0", "[Transport]")
{
    Transport t;
    t.play();
    t.advancePosition(1000);
    REQUIRE(t.getSamplePosition() == 1000);

    t.stop();
    REQUIRE(t.getSamplePosition() == 0);
}

TEST_CASE("Transport pause preserves position", "[Transport]")
{
    Transport t;
    t.play();
    t.advancePosition(1000);

    t.pause();
    REQUIRE(t.getSamplePosition() == 1000);
}

TEST_CASE("Transport BPM defaults to 120", "[Transport]")
{
    Transport t;
    REQUIRE_THAT(t.getBpm(), WithinAbs(120.0, 0.001));
}

TEST_CASE("Transport setBpm changes BPM", "[Transport]")
{
    Transport t;
    t.setBpm(140.0);
    REQUIRE_THAT(t.getBpm(), WithinAbs(140.0, 0.001));
}

TEST_CASE("Transport time signature defaults to 4/4", "[Transport]")
{
    Transport t;
    REQUIRE(t.getTimeSignatureNumerator() == 4);
    REQUIRE(t.getTimeSignatureDenominator() == 4);
}

TEST_CASE("Transport setTimeSignature works", "[Transport]")
{
    Transport t;
    t.setTimeSignature(3, 4);
    REQUIRE(t.getTimeSignatureNumerator() == 3);
    REQUIRE(t.getTimeSignatureDenominator() == 4);
}

TEST_CASE("Transport advancePosition increments sample position", "[Transport]")
{
    Transport t;
    t.setSampleRate(44100.0);
    t.play();

    t.advancePosition(512);
    REQUIRE(t.getSamplePosition() == 512);

    t.advancePosition(512);
    REQUIRE(t.getSamplePosition() == 1024);
}

TEST_CASE("Transport advancePosition does nothing when stopped", "[Transport]")
{
    Transport t;
    t.advancePosition(512);
    REQUIRE(t.getSamplePosition() == 0);
}

TEST_CASE("Transport getPosition returns valid PositionInfo", "[Transport]")
{
    Transport t;
    t.play();
    t.setBpm(120.0);
    t.setSampleRate(44100.0);

    // Advance 1 second worth of samples
    t.advancePosition(44100);

    auto pos = t.getPosition();
    REQUIRE(pos.hasValue());

    // At 120 BPM, 1 second = 2 beats
    REQUIRE_THAT(*pos->getBpm(), WithinAbs(120.0, 0.001));
    REQUIRE_THAT(*pos->getPpqPosition(), WithinAbs(2.0, 0.01));
    REQUIRE(pos->getIsPlaying());
}

TEST_CASE("Transport loop wraps position", "[Transport]")
{
    Transport t;
    t.setSampleRate(44100.0);
    // 60 BPM = 1 beat/sec = 44100 samples/beat
    t.setBpm(60.0);
    // Loop 0-2 beats = 0-88200 samples
    t.setLoopRegion(0.0, 2.0, true);

    t.play();
    t.advancePosition(88200);

    // Position should wrap back to 0 (start of loop)
    REQUIRE(t.getSamplePosition() == 0);
}

TEST_CASE("Transport loop disabled does not wrap", "[Transport]")
{
    Transport t;
    t.setSampleRate(44100.0);
    t.setBpm(60.0);
    t.setLoopRegion(0.0, 2.0, false);

    t.play();
    t.advancePosition(88200);

    // Position should NOT wrap since looping is disabled
    REQUIRE(t.getSamplePosition() == 88200);
}

TEST_CASE("Transport loop with non-zero start wraps correctly", "[Transport]")
{
    Transport t;
    t.setSampleRate(44100.0);
    t.setBpm(60.0);
    // Loop from beat 1 to beat 3 (44100 to 132300 samples)
    t.setLoopRegion(1.0, 3.0, true);

    t.play();
    // Advance to beat 3 exactly (132300 samples)
    t.advancePosition(132300);

    // Should wrap to loop start (beat 1 = 44100 samples)
    REQUIRE(t.getSamplePosition() == 44100);
}
