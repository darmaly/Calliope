#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>

namespace calliope {

class Command : public juce::UndoableAction {
public:
    virtual ~Command() = default;

    // Name used for undo descriptions and event notifications
    virtual juce::String getCommandName() const = 0;

    // Data emitted with command events
    virtual juce::var getEventData() const = 0;

    // Override to return false for non-undoable commands (e.g., transport play/stop)
    virtual bool isUndoable() const { return true; }
};

} // namespace calliope
