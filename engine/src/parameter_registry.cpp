#include "calliope/parameter_registry.h"

namespace calliope {

void ParameterRegistry::registerParameter(const juce::String& id, ParameterDef def)
{
    params_[id] = std::move(def);
}

void ParameterRegistry::removeParameter(const juce::String& id)
{
    params_.erase(id);
}

void ParameterRegistry::removeParametersWithPrefix(const juce::String& prefix)
{
    for (auto it = params_.begin(); it != params_.end(); ) {
        if (it->first.startsWith(prefix))
            it = params_.erase(it);
        else
            ++it;
    }
}

const ParameterRegistry::ParameterDef* ParameterRegistry::getParameter(const juce::String& id) const
{
    auto it = params_.find(id);
    if (it != params_.end())
        return &it->second;
    return nullptr;
}

std::vector<juce::String> ParameterRegistry::getAllParameterIds() const
{
    std::vector<juce::String> ids;
    ids.reserve(params_.size());
    for (const auto& [key, value] : params_)
        ids.push_back(key);
    return ids;
}

} // namespace calliope
