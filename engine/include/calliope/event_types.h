#pragma once
#include <juce_core/juce_core.h>

namespace calliope {

enum class EventType {
    CommandExecuted,
    CommandUndone,
    CommandRedone,
    ParameterChanged,
    TransportStateChanged,
    ProjectStateChanged
};

struct CommandEvent {
    EventType type;
    juce::String commandName;
    juce::var data;
    bool isUndo = false;
};

} // namespace calliope
