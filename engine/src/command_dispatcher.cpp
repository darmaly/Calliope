#include "calliope/command_dispatcher.h"

namespace calliope {

CommandDispatcher::CommandDispatcher()
    : undoManager_(0, 200) // 0 = no size-in-units limit, 200 = max transactions
{
}

CommandDispatcher::~CommandDispatcher() = default;

bool CommandDispatcher::dispatch(std::unique_ptr<Command> command)
{
    if (!command)
        return false;

    // Capture name and event data before potential ownership transfer
    juce::String name = command->getCommandName();
    juce::var data = command->getEventData();

    if (command->isUndoable())
    {
        undoManager_.beginNewTransaction(name);
        // UndoManager takes ownership via raw pointer
        bool result = undoManager_.perform(command.release());
        if (result)
            notifyExecuted(name, data);
        return result;
    }
    else
    {
        // Non-undoable: execute directly, unique_ptr auto-deletes
        bool result = command->perform();
        if (result)
            notifyExecuted(name, data);
        return result;
    }
}

bool CommandDispatcher::undo()
{
    if (!canUndo())
        return false;

    // Capture the description before undoing (it will change after)
    juce::String name = undoManager_.getUndoDescription();

    bool result = undoManager_.undo();
    if (result)
        notifyUndone(name, juce::var());

    return result;
}

bool CommandDispatcher::redo()
{
    if (!canRedo())
        return false;

    // Capture the description before redoing
    juce::String name = undoManager_.getRedoDescription();

    bool result = undoManager_.redo();
    if (result)
        notifyExecuted(name, juce::var());

    return result;
}

bool CommandDispatcher::canUndo() const
{
    return undoManager_.canUndo();
}

bool CommandDispatcher::canRedo() const
{
    return undoManager_.canRedo();
}

juce::String CommandDispatcher::getUndoDescription() const
{
    return undoManager_.getUndoDescription();
}

juce::String CommandDispatcher::getRedoDescription() const
{
    return undoManager_.getRedoDescription();
}

void CommandDispatcher::addListener(Listener* listener)
{
    listeners_.add(listener);
}

void CommandDispatcher::removeListener(Listener* listener)
{
    listeners_.remove(listener);
}

void CommandDispatcher::notifyExecuted(const juce::String& name, const juce::var& data)
{
    CommandEvent event;
    event.type = EventType::CommandExecuted;
    event.commandName = name;
    event.data = data;
    event.isUndo = false;
    listeners_.call(&Listener::commandExecuted, event);
}

void CommandDispatcher::notifyUndone(const juce::String& name, const juce::var& data)
{
    CommandEvent event;
    event.type = EventType::CommandUndone;
    event.commandName = name;
    event.data = data;
    event.isUndo = true;
    listeners_.call(&Listener::commandUndone, event);
}

} // namespace calliope
