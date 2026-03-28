#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <map>
#include <vector>

namespace calliope {

class ParameterRegistry {
public:
    struct ParameterDef {
        std::function<juce::var()> getter;
        std::function<void(const juce::var&)> setter;
        juce::String type;    // "float", "double", "int", "bool"
        juce::var minValue;
        juce::var maxValue;
    };

    void registerParameter(const juce::String& id, ParameterDef def);
    const ParameterDef* getParameter(const juce::String& id) const;
    std::vector<juce::String> getAllParameterIds() const;

private:
    std::map<juce::String, ParameterDef> params_;
};

} // namespace calliope
