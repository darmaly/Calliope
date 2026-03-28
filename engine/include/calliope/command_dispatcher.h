#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include "calliope/command.h"
#include "calliope/event_types.h"
#include <memory>

namespace calliope {

class CommandDispatcher {
public:
    CommandDispatcher();
    ~CommandDispatcher();

    // Execute a command. If undoable, goes through UndoManager.
    // If not undoable, executed directly.
    // Returns true if command was performed.
    bool dispatch(std::unique_ptr<Command> command);

    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;

    // Listener for command events
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void commandExecuted(const CommandEvent& event) = 0;
        virtual void commandUndone(const CommandEvent& event) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    juce::UndoManager undoManager_;
    juce::ListenerList<Listener> listeners_;

    void notifyExecuted(const juce::String& name, const juce::var& data);
    void notifyUndone(const juce::String& name, const juce::var& data);
};

} // namespace calliope
