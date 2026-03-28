#pragma once
#include "calliope/command.h"
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/drum_machine.h"

namespace calliope {

// Forward declaration
class Engine;

class NoteOnCommand : public Command {
public:
    // Constructor taking a processor reference directly
    NoteOnCommand(juce::AudioProcessor& processor, int note, float velocity)
        : processor_(processor), note_(note), velocity_(velocity) {}

    bool perform() override;
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "instrument.noteOn"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return false; }

private:
    juce::AudioProcessor& processor_;
    int note_;
    float velocity_;
};

class NoteOffCommand : public Command {
public:
    NoteOffCommand(juce::AudioProcessor& processor, int note)
        : processor_(processor), note_(note) {}

    bool perform() override;
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "instrument.noteOff"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return false; }

private:
    juce::AudioProcessor& processor_;
    int note_;
};

class LoadSampleCommand : public Command {
public:
    LoadSampleCommand(DrumMachineProcessor& drumMachine, int padIndex, const std::string& filePath)
        : drumMachine_(drumMachine), padIndex_(padIndex), filePath_(filePath) {}

    bool perform() override;
    bool undo() override { return false; }
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "drumMachine.loadSample"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return false; }

private:
    DrumMachineProcessor& drumMachine_;
    int padIndex_;
    std::string filePath_;
};

} // namespace calliope
