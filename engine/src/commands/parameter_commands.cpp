#include "calliope/commands/parameter_commands.h"

namespace calliope {

bool SetParameterCommand::perform() {
    const auto* def = registry_.getParameter(paramId_);
    if (!def) return false;

    oldValue_ = def->getter();
    def->setter(newValue_);
    return true;
}

bool SetParameterCommand::undo() {
    const auto* def = registry_.getParameter(paramId_);
    if (!def) return false;

    def->setter(oldValue_);
    return true;
}

juce::var SetParameterCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("parameterId", paramId_);
    obj->setProperty("value", newValue_);
    return juce::var(obj);
}

} // namespace calliope
