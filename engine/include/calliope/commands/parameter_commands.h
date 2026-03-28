#pragma once
#include "calliope/command.h"
#include "calliope/parameter_registry.h"

namespace calliope {

class SetParameterCommand : public Command {
public:
    SetParameterCommand(ParameterRegistry& registry, const juce::String& paramId, juce::var newValue)
        : registry_(registry), paramId_(paramId), newValue_(std::move(newValue)) {}

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "SetParameter:" + paramId_; }
    juce::var getEventData() const override;

private:
    ParameterRegistry& registry_;
    juce::String paramId_;
    juce::var newValue_;
    juce::var oldValue_;
};

} // namespace calliope
