#pragma once
#include "calliope/command.h"
#include "calliope/insert_chain.h"
#include <memory>

namespace calliope {

// Forward declarations
class Engine;

// Factory function to create effect processors by type name
std::unique_ptr<juce::AudioProcessor> createEffect(const juce::String& type);

// --------------------------------------------------------------------------
// InsertEffectCommand -- inserts a new effect into an InsertChain
// --------------------------------------------------------------------------
class InsertEffectCommand : public Command {
public:
    InsertEffectCommand(InsertChain& chain, Engine& engine,
                        const juce::String& trackId,
                        const juce::String& effectType,
                        int position = -1);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "effect.insert"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return true; }

private:
    InsertChain& chain_;
    Engine& engine_;
    juce::String trackId_;
    juce::String effectType_;
    int requestedPosition_;
    int insertedIndex_ = -1;
    std::unique_ptr<juce::AudioProcessor> removedEffect_;
};

// --------------------------------------------------------------------------
// RemoveEffectCommand -- removes an effect from an InsertChain
// --------------------------------------------------------------------------
class RemoveEffectCommand : public Command {
public:
    RemoveEffectCommand(InsertChain& chain, Engine& engine,
                        const juce::String& trackId, int position);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "effect.remove"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return true; }

private:
    InsertChain& chain_;
    Engine& engine_;
    juce::String trackId_;
    int position_;
    juce::String effectType_;
    std::unique_ptr<juce::AudioProcessor> removedEffect_;
};

// --------------------------------------------------------------------------
// ReorderEffectCommand -- moves an effect within an InsertChain
// --------------------------------------------------------------------------
class ReorderEffectCommand : public Command {
public:
    ReorderEffectCommand(InsertChain& chain, Engine& engine,
                         const juce::String& trackId,
                         int fromPosition, int toPosition);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "effect.reorder"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return true; }

private:
    InsertChain& chain_;
    Engine& engine_;
    juce::String trackId_;
    int fromPosition_;
    int toPosition_;

    void reRegisterAllParameters();
};

// --------------------------------------------------------------------------
// BypassEffectCommand -- toggles bypass on an effect slot
// --------------------------------------------------------------------------
class BypassEffectCommand : public Command {
public:
    BypassEffectCommand(InsertChain& chain, int position, bool bypassed);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "effect.bypass"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return true; }

private:
    InsertChain& chain_;
    int position_;
    bool bypassed_;
    bool previousBypassed_ = false;
};

} // namespace calliope
