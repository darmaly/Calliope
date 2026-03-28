#include "calliope/commands/effect_commands.h"
#include "calliope/engine.h"
#include "calliope/effects/parametric_eq.h"
#include "calliope/effects/compressor.h"
#include "calliope/effects/reverb.h"
#include "calliope/effects/delay.h"
#include "calliope/effects/limiter.h"

namespace calliope {

// --------------------------------------------------------------------------
// Factory
// --------------------------------------------------------------------------

std::unique_ptr<juce::AudioProcessor> createEffect(const juce::String& type)
{
    if (type == "eq")         return std::make_unique<ParametricEqProcessor>();
    if (type == "compressor") return std::make_unique<CompressorProcessor>();
    if (type == "reverb")     return std::make_unique<ReverbProcessor>();
    if (type == "delay")      return std::make_unique<DelayProcessor>();
    if (type == "limiter")    return std::make_unique<LimiterProcessor>();
    return nullptr;
}

// --------------------------------------------------------------------------
// InsertEffectCommand
// --------------------------------------------------------------------------

InsertEffectCommand::InsertEffectCommand(InsertChain& chain, Engine& engine,
                                         const juce::String& trackId,
                                         const juce::String& effectType,
                                         int position)
    : chain_(chain), engine_(engine), trackId_(trackId),
      effectType_(effectType), requestedPosition_(position)
{
}

bool InsertEffectCommand::perform()
{
    std::unique_ptr<juce::AudioProcessor> effect;

    if (removedEffect_) {
        // Re-doing: reuse the previously removed effect
        effect = std::move(removedEffect_);
    } else {
        effect = createEffect(effectType_);
        if (!effect) return false;
    }

    insertedIndex_ = chain_.insertEffect(std::move(effect), requestedPosition_);
    engine_.registerEffectParameters(trackId_, insertedIndex_, chain_.getEffect(insertedIndex_));
    return true;
}

bool InsertEffectCommand::undo()
{
    if (insertedIndex_ < 0) return false;
    engine_.unregisterEffectParameters(trackId_, insertedIndex_);
    removedEffect_ = chain_.removeEffect(insertedIndex_);
    return removedEffect_ != nullptr;
}

juce::var InsertEffectCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("effectType", effectType_);
    obj->setProperty("trackId", trackId_);
    obj->setProperty("position", insertedIndex_);
    return juce::var(obj);
}

// --------------------------------------------------------------------------
// RemoveEffectCommand
// --------------------------------------------------------------------------

RemoveEffectCommand::RemoveEffectCommand(InsertChain& chain, Engine& engine,
                                         const juce::String& trackId, int position)
    : chain_(chain), engine_(engine), trackId_(trackId), position_(position)
{
}

bool RemoveEffectCommand::perform()
{
    auto* effect = chain_.getEffect(position_);
    if (!effect) return false;

    effectType_ = effect->getName();
    engine_.unregisterEffectParameters(trackId_, position_);
    removedEffect_ = chain_.removeEffect(position_);
    return removedEffect_ != nullptr;
}

bool RemoveEffectCommand::undo()
{
    if (!removedEffect_) return false;

    int idx = chain_.insertEffect(std::move(removedEffect_), position_);
    engine_.registerEffectParameters(trackId_, idx, chain_.getEffect(idx));
    return true;
}

juce::var RemoveEffectCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("effectType", effectType_);
    obj->setProperty("trackId", trackId_);
    obj->setProperty("position", position_);
    return juce::var(obj);
}

// --------------------------------------------------------------------------
// ReorderEffectCommand
// --------------------------------------------------------------------------

ReorderEffectCommand::ReorderEffectCommand(InsertChain& chain, Engine& engine,
                                           const juce::String& trackId,
                                           int fromPosition, int toPosition)
    : chain_(chain), engine_(engine), trackId_(trackId),
      fromPosition_(fromPosition), toPosition_(toPosition)
{
}

bool ReorderEffectCommand::perform()
{
    chain_.moveEffect(fromPosition_, toPosition_);
    reRegisterAllParameters();
    return true;
}

bool ReorderEffectCommand::undo()
{
    chain_.moveEffect(toPosition_, fromPosition_);
    reRegisterAllParameters();
    return true;
}

void ReorderEffectCommand::reRegisterAllParameters()
{
    // Unregister all effect parameters for this track
    engine_.getParameterRegistry().removeParametersWithPrefix("effects." + trackId_ + ".");

    // Re-register all effects with their current slot indices
    int numEffects = chain_.getNumEffects();
    for (int i = 0; i < numEffects; ++i) {
        engine_.registerEffectParameters(trackId_, i, chain_.getEffect(i));
    }
}

juce::var ReorderEffectCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("trackId", trackId_);
    obj->setProperty("fromPosition", fromPosition_);
    obj->setProperty("toPosition", toPosition_);
    return juce::var(obj);
}

// --------------------------------------------------------------------------
// BypassEffectCommand
// --------------------------------------------------------------------------

BypassEffectCommand::BypassEffectCommand(InsertChain& chain, int position, bool bypassed)
    : chain_(chain), position_(position), bypassed_(bypassed)
{
}

bool BypassEffectCommand::perform()
{
    previousBypassed_ = chain_.isBypassed(position_);
    chain_.setBypass(position_, bypassed_);
    return true;
}

bool BypassEffectCommand::undo()
{
    chain_.setBypass(position_, previousBypassed_);
    return true;
}

juce::var BypassEffectCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("position", position_);
    obj->setProperty("bypassed", bypassed_);
    return juce::var(obj);
}

} // namespace calliope
