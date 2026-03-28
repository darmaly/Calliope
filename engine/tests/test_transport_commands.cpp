#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/commands/transport_commands.h"
#include "calliope/transport.h"
#include "calliope/metronome.h"
#include "calliope/master_bus.h"

using namespace calliope;

// --- PlayCommand ---

TEST_CASE("PlayCommand::perform() calls Transport::play(), returns true", "[transport_commands]") {
    Transport transport;
    PlayCommand cmd(transport);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(transport.getState() == TransportState::Playing);
}

TEST_CASE("PlayCommand::isUndoable() returns false", "[transport_commands]") {
    Transport transport;
    PlayCommand cmd(transport);

    REQUIRE(cmd.isUndoable() == false);
}

// --- StopCommand ---

TEST_CASE("StopCommand::perform() calls Transport::stop(), returns true", "[transport_commands]") {
    Transport transport;
    transport.play();
    StopCommand cmd(transport);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(transport.getState() == TransportState::Stopped);
}

TEST_CASE("StopCommand::isUndoable() returns false", "[transport_commands]") {
    Transport transport;
    StopCommand cmd(transport);

    REQUIRE(cmd.isUndoable() == false);
}

// --- PauseCommand ---

TEST_CASE("PauseCommand::perform() calls Transport::pause(), returns true", "[transport_commands]") {
    Transport transport;
    transport.play();
    PauseCommand cmd(transport);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(transport.getState() == TransportState::Paused);
}

TEST_CASE("PauseCommand::isUndoable() returns false", "[transport_commands]") {
    Transport transport;
    PauseCommand cmd(transport);

    REQUIRE(cmd.isUndoable() == false);
}

// --- SetBpmCommand ---

TEST_CASE("SetBpmCommand::perform() sets BPM on Transport, returns true", "[transport_commands]") {
    Transport transport;
    REQUIRE(transport.getBpm() == 120.0);

    SetBpmCommand cmd(transport, 140.0);
    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(transport.getBpm() == 140.0);
}

TEST_CASE("SetBpmCommand::undo() restores previous BPM", "[transport_commands]") {
    Transport transport;
    transport.setBpm(100.0);

    SetBpmCommand cmd(transport, 140.0);
    cmd.perform();
    REQUIRE(transport.getBpm() == 140.0);

    cmd.undo();
    REQUIRE(transport.getBpm() == 100.0);
}

TEST_CASE("SetBpmCommand::isUndoable() returns true", "[transport_commands]") {
    Transport transport;
    SetBpmCommand cmd(transport, 140.0);

    REQUIRE(cmd.isUndoable() == true);
}

TEST_CASE("SetBpmCommand::getEventData() contains bpm property", "[transport_commands]") {
    Transport transport;
    SetBpmCommand cmd(transport, 140.0);

    auto data = cmd.getEventData();
    REQUIRE(data.hasProperty("bpm"));
    REQUIRE(static_cast<double>(data["bpm"]) == 140.0);
}

// --- SetTimeSignatureCommand ---

TEST_CASE("SetTimeSignatureCommand::perform() sets numerator and denominator", "[transport_commands]") {
    Transport transport;
    SetTimeSignatureCommand cmd(transport, 3, 8);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(transport.getTimeSignatureNumerator() == 3);
    REQUIRE(transport.getTimeSignatureDenominator() == 8);
}

TEST_CASE("SetTimeSignatureCommand::undo() restores previous time signature", "[transport_commands]") {
    Transport transport;
    // Default is 4/4
    SetTimeSignatureCommand cmd(transport, 3, 8);
    cmd.perform();

    cmd.undo();
    REQUIRE(transport.getTimeSignatureNumerator() == 4);
    REQUIRE(transport.getTimeSignatureDenominator() == 4);
}

// --- SetLoopRegionCommand ---

TEST_CASE("SetLoopRegionCommand::perform() sets loop start/end/enabled", "[transport_commands]") {
    Transport transport;
    SetLoopRegionCommand cmd(transport, 4.0, 16.0, true);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(transport.getLoopStartBeat() == 4.0);
    REQUIRE(transport.getLoopEndBeat() == 16.0);
    REQUIRE(transport.isLooping() == true);
}

TEST_CASE("SetLoopRegionCommand::undo() restores previous loop settings", "[transport_commands]") {
    Transport transport;
    // defaults: 0.0, 0.0, false
    SetLoopRegionCommand cmd(transport, 4.0, 16.0, true);
    cmd.perform();

    cmd.undo();
    REQUIRE(transport.getLoopStartBeat() == 0.0);
    REQUIRE(transport.getLoopEndBeat() == 0.0);
    REQUIRE(transport.isLooping() == false);
}

// --- SetMetronomeEnabledCommand ---

TEST_CASE("SetMetronomeEnabledCommand::perform() sets enabled flag", "[transport_commands]") {
    MetronomeProcessor metronome;
    metronome.enabled.store(true);
    SetMetronomeEnabledCommand cmd(metronome, false);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(metronome.enabled.load() == false);
}

TEST_CASE("SetMetronomeEnabledCommand::undo() restores previous enabled state", "[transport_commands]") {
    MetronomeProcessor metronome;
    metronome.enabled.store(true);
    SetMetronomeEnabledCommand cmd(metronome, false);
    cmd.perform();

    cmd.undo();
    REQUIRE(metronome.enabled.load() == true);
}

// --- SetMetronomeVolumeCommand ---

TEST_CASE("SetMetronomeVolumeCommand::perform() sets volume", "[transport_commands]") {
    MetronomeProcessor metronome;
    metronome.volume.store(0.7f);
    SetMetronomeVolumeCommand cmd(metronome, 0.5f);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(metronome.volume.load() == 0.5f);
}

TEST_CASE("SetMetronomeVolumeCommand::undo() restores previous volume", "[transport_commands]") {
    MetronomeProcessor metronome;
    metronome.volume.store(0.7f);
    SetMetronomeVolumeCommand cmd(metronome, 0.5f);
    cmd.perform();

    cmd.undo();
    REQUIRE(metronome.volume.load() == 0.7f);
}

// --- SetMasterVolumeCommand ---

TEST_CASE("SetMasterVolumeCommand::perform() sets master volume", "[transport_commands]") {
    MasterBusProcessor masterBus;
    masterBus.masterVolume.store(1.0f);
    SetMasterVolumeCommand cmd(masterBus, 0.8f);

    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(masterBus.masterVolume.load() == 0.8f);
}

TEST_CASE("SetMasterVolumeCommand::undo() restores previous volume", "[transport_commands]") {
    MasterBusProcessor masterBus;
    masterBus.masterVolume.store(1.0f);
    SetMasterVolumeCommand cmd(masterBus, 0.8f);
    cmd.perform();

    cmd.undo();
    REQUIRE(masterBus.masterVolume.load() == 1.0f);
}
