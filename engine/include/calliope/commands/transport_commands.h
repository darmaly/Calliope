#pragma once
#include "calliope/command.h"
#include "calliope/transport.h"
#include "calliope/metronome.h"
#include "calliope/master_bus.h"

namespace calliope {

// --- Non-undoable transport commands ---

class PlayCommand : public Command {
public:
    explicit PlayCommand(Transport& transport) : transport_(transport) {}

    bool perform() override;
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "Play"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return false; }

private:
    Transport& transport_;
};

class StopCommand : public Command {
public:
    explicit StopCommand(Transport& transport) : transport_(transport) {}

    bool perform() override;
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "Stop"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return false; }

private:
    Transport& transport_;
};

class PauseCommand : public Command {
public:
    explicit PauseCommand(Transport& transport) : transport_(transport) {}

    bool perform() override;
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "Pause"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return false; }

private:
    Transport& transport_;
};

// --- Undoable transport commands ---

class SetBpmCommand : public Command {
public:
    SetBpmCommand(Transport& transport, double newBpm)
        : transport_(transport), newBpm_(newBpm) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetBpm"; }
    juce::var getEventData() const override;

private:
    Transport& transport_;
    double newBpm_;
    double oldBpm_ = 0.0;
};

class SetTimeSignatureCommand : public Command {
public:
    SetTimeSignatureCommand(Transport& transport, int newNum, int newDen)
        : transport_(transport), newNum_(newNum), newDen_(newDen) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetTimeSignature"; }
    juce::var getEventData() const override;

private:
    Transport& transport_;
    int newNum_, newDen_;
    int oldNum_ = 4, oldDen_ = 4;
};

class SetLoopRegionCommand : public Command {
public:
    SetLoopRegionCommand(Transport& transport, double startBeat, double endBeat, bool enabled)
        : transport_(transport), newStart_(startBeat), newEnd_(endBeat), newEnabled_(enabled) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetLoopRegion"; }
    juce::var getEventData() const override;

private:
    Transport& transport_;
    double newStart_, newEnd_;
    bool newEnabled_;
    double oldStart_ = 0.0, oldEnd_ = 0.0;
    bool oldEnabled_ = false;
};

// --- Metronome commands ---

class SetMetronomeEnabledCommand : public Command {
public:
    SetMetronomeEnabledCommand(MetronomeProcessor& metronome, bool enabled)
        : metronome_(metronome), newEnabled_(enabled) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetMetronomeEnabled"; }
    juce::var getEventData() const override;

private:
    MetronomeProcessor& metronome_;
    bool newEnabled_;
    bool oldEnabled_ = true;
};

class SetMetronomeVolumeCommand : public Command {
public:
    SetMetronomeVolumeCommand(MetronomeProcessor& metronome, float volume)
        : metronome_(metronome), newVolume_(volume) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetMetronomeVolume"; }
    juce::var getEventData() const override;

private:
    MetronomeProcessor& metronome_;
    float newVolume_;
    float oldVolume_ = 0.7f;
};

// --- Master bus commands ---

class SetMasterVolumeCommand : public Command {
public:
    SetMasterVolumeCommand(MasterBusProcessor& masterBus, float volume)
        : masterBus_(masterBus), newVolume_(volume) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetMasterVolume"; }
    juce::var getEventData() const override;

private:
    MasterBusProcessor& masterBus_;
    float newVolume_;
    float oldVolume_ = 1.0f;
};

} // namespace calliope
