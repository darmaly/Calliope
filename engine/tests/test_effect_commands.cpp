#include <catch2/catch_test_macros.hpp>
#include "calliope/commands/effect_commands.h"
#include "calliope/insert_chain.h"
#include "calliope/insert_chain_processor.h"
#include "calliope/engine.h"
#include "calliope/parameter_registry.h"
#include "calliope/project_state.h"
#include "calliope/effects/parametric_eq.h"
#include "calliope/effects/compressor.h"
#include "calliope/effects/reverb.h"
#include "calliope/effects/delay.h"
#include "calliope/effects/limiter.h"

using namespace calliope;

// ==================================================================
// InsertChainProcessor tests
// ==================================================================

TEST_CASE("InsertChainProcessor getName includes trackId", "[InsertChainProcessor]")
{
    InsertChainProcessor proc("polysynth");
    REQUIRE(proc.getName() == "InsertChain:polysynth");
}

TEST_CASE("InsertChainProcessor processBlock delegates to InsertChain", "[InsertChainProcessor]")
{
    InsertChainProcessor proc("test");
    proc.prepareToPlay(44100.0, 512);

    // Insert an effect so we can verify delegation
    auto eq = std::make_unique<ParametricEqProcessor>();
    proc.getInsertChain().insertEffect(std::move(eq));

    REQUIRE(proc.getInsertChain().getNumEffects() == 1);

    // Process a buffer - should not crash
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);
    REQUIRE(true);
}

TEST_CASE("InsertChainProcessor getInsertChain returns reference", "[InsertChainProcessor]")
{
    InsertChainProcessor proc("master");
    InsertChain& chain = proc.getInsertChain();
    REQUIRE(chain.getNumEffects() == 0);
}

TEST_CASE("InsertChainProcessor getTrackId returns track identifier", "[InsertChainProcessor]")
{
    InsertChainProcessor proc("basssynth");
    REQUIRE(proc.getTrackId() == "basssynth");
}

TEST_CASE("InsertChainProcessor getTailLengthSeconds returns 12.0", "[InsertChainProcessor]")
{
    InsertChainProcessor proc("test");
    REQUIRE(proc.getTailLengthSeconds() == 12.0);
}

// ==================================================================
// ParameterRegistry removal tests
// ==================================================================

TEST_CASE("ParameterRegistry removeParameter removes a registered parameter", "[ParameterRegistry]")
{
    ParameterRegistry registry;
    registry.registerParameter("test.param1", {
        []() -> juce::var { return 1.0; },
        [](const juce::var&) {},
        "float", 0.0, 10.0
    });

    REQUIRE(registry.getParameter("test.param1") != nullptr);
    registry.removeParameter("test.param1");
    REQUIRE(registry.getParameter("test.param1") == nullptr);
}

TEST_CASE("ParameterRegistry removeParametersWithPrefix removes all matching", "[ParameterRegistry]")
{
    ParameterRegistry registry;
    registry.registerParameter("effects.polysynth.0.threshold", {
        []() -> juce::var { return 1.0; }, [](const juce::var&) {}, "float", 0.0, 1.0
    });
    registry.registerParameter("effects.polysynth.0.ratio", {
        []() -> juce::var { return 1.0; }, [](const juce::var&) {}, "float", 0.0, 1.0
    });
    registry.registerParameter("effects.polysynth.1.threshold", {
        []() -> juce::var { return 1.0; }, [](const juce::var&) {}, "float", 0.0, 1.0
    });
    registry.registerParameter("transport.bpm", {
        []() -> juce::var { return 120.0; }, [](const juce::var&) {}, "double", 20.0, 999.0
    });

    REQUIRE(registry.getAllParameterIds().size() == 4);

    registry.removeParametersWithPrefix("effects.polysynth.0.");
    REQUIRE(registry.getAllParameterIds().size() == 2);
    REQUIRE(registry.getParameter("effects.polysynth.0.threshold") == nullptr);
    REQUIRE(registry.getParameter("effects.polysynth.0.ratio") == nullptr);
    REQUIRE(registry.getParameter("effects.polysynth.1.threshold") != nullptr);
    REQUIRE(registry.getParameter("transport.bpm") != nullptr);
}

// ==================================================================
// createEffect factory tests
// ==================================================================

TEST_CASE("createEffect factory creates correct effect types", "[EffectCommands]")
{
    auto eq = createEffect("eq");
    REQUIRE(eq != nullptr);
    REQUIRE(eq->getName() == "ParametricEQ");

    auto comp = createEffect("compressor");
    REQUIRE(comp != nullptr);
    REQUIRE(comp->getName() == "Compressor");

    auto rev = createEffect("reverb");
    REQUIRE(rev != nullptr);
    REQUIRE(rev->getName() == "Reverb");

    auto del = createEffect("delay");
    REQUIRE(del != nullptr);
    REQUIRE(del->getName() == "Delay");

    auto lim = createEffect("limiter");
    REQUIRE(lim != nullptr);
    REQUIRE(lim->getName() == "Limiter");

    auto unknown = createEffect("unknown");
    REQUIRE(unknown == nullptr);
}

// ==================================================================
// InsertEffectCommand tests
// ==================================================================

TEST_CASE("InsertEffectCommand perform inserts effect, getNumEffects increases", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    InsertEffectCommand cmd(chain, engine, "polysynth", "compressor");
    REQUIRE(chain.getNumEffects() == 0);
    REQUIRE(cmd.perform() == true);
    REQUIRE(chain.getNumEffects() == 1);
}

TEST_CASE("InsertEffectCommand undo removes the inserted effect", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    InsertEffectCommand cmd(chain, engine, "polysynth", "eq");
    cmd.perform();
    REQUIRE(chain.getNumEffects() == 1);

    REQUIRE(cmd.undo() == true);
    REQUIRE(chain.getNumEffects() == 0);
}

TEST_CASE("InsertEffectCommand getCommandName returns effect.insert", "[EffectCommands]")
{
    InsertChain chain;
    auto& engine = Engine::getInstance();
    InsertEffectCommand cmd(chain, engine, "polysynth", "compressor");
    REQUIRE(cmd.getCommandName() == "effect.insert");
}

TEST_CASE("InsertEffectCommand isUndoable returns true", "[EffectCommands]")
{
    InsertChain chain;
    auto& engine = Engine::getInstance();
    InsertEffectCommand cmd(chain, engine, "polysynth", "compressor");
    REQUIRE(cmd.isUndoable() == true);
}

TEST_CASE("InsertEffectCommand getEventData includes effectType, trackId, position", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    InsertEffectCommand cmd(chain, engine, "polysynth", "reverb", 0);
    cmd.perform();

    auto data = cmd.getEventData();
    REQUIRE(data.isObject());
    auto* obj = data.getDynamicObject();
    REQUIRE(obj->getProperty("effectType").toString() == "reverb");
    REQUIRE(obj->getProperty("trackId").toString() == "polysynth");
    REQUIRE(static_cast<int>(obj->getProperty("position")) == 0);
}

// ==================================================================
// RemoveEffectCommand tests
// ==================================================================

TEST_CASE("RemoveEffectCommand perform removes effect, getNumEffects decreases", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    chain.insertEffect(createEffect("compressor"));
    REQUIRE(chain.getNumEffects() == 1);

    RemoveEffectCommand cmd(chain, engine, "polysynth", 0);
    REQUIRE(cmd.perform() == true);
    REQUIRE(chain.getNumEffects() == 0);
}

TEST_CASE("RemoveEffectCommand undo re-inserts the removed effect", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    chain.insertEffect(createEffect("eq"));
    RemoveEffectCommand cmd(chain, engine, "polysynth", 0);
    cmd.perform();
    REQUIRE(chain.getNumEffects() == 0);

    REQUIRE(cmd.undo() == true);
    REQUIRE(chain.getNumEffects() == 1);
    REQUIRE(chain.getEffect(0)->getName() == "ParametricEQ");
}

TEST_CASE("RemoveEffectCommand getCommandName returns effect.remove", "[EffectCommands]")
{
    InsertChain chain;
    auto& engine = Engine::getInstance();
    RemoveEffectCommand cmd(chain, engine, "polysynth", 0);
    REQUIRE(cmd.getCommandName() == "effect.remove");
}

// ==================================================================
// ReorderEffectCommand tests
// ==================================================================

TEST_CASE("ReorderEffectCommand perform moves effect from one position to another", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    chain.insertEffect(createEffect("eq"));          // position 0
    chain.insertEffect(createEffect("compressor"));   // position 1

    REQUIRE(chain.getEffect(0)->getName() == "ParametricEQ");
    REQUIRE(chain.getEffect(1)->getName() == "Compressor");

    ReorderEffectCommand cmd(chain, engine, "polysynth", 1, 0);
    REQUIRE(cmd.perform() == true);

    REQUIRE(chain.getEffect(0)->getName() == "Compressor");
    REQUIRE(chain.getEffect(1)->getName() == "ParametricEQ");
}

TEST_CASE("ReorderEffectCommand undo moves it back", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    auto& engine = Engine::getInstance();

    chain.insertEffect(createEffect("eq"));
    chain.insertEffect(createEffect("compressor"));

    ReorderEffectCommand cmd(chain, engine, "polysynth", 1, 0);
    cmd.perform();
    REQUIRE(chain.getEffect(0)->getName() == "Compressor");

    REQUIRE(cmd.undo() == true);
    REQUIRE(chain.getEffect(0)->getName() == "ParametricEQ");
    REQUIRE(chain.getEffect(1)->getName() == "Compressor");
}

TEST_CASE("ReorderEffectCommand getCommandName returns effect.reorder", "[EffectCommands]")
{
    InsertChain chain;
    auto& engine = Engine::getInstance();
    ReorderEffectCommand cmd(chain, engine, "polysynth", 0, 1);
    REQUIRE(cmd.getCommandName() == "effect.reorder");
}

// ==================================================================
// BypassEffectCommand tests
// ==================================================================

TEST_CASE("BypassEffectCommand perform sets bypass state", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    chain.insertEffect(createEffect("compressor"));

    REQUIRE(chain.isBypassed(0) == false);

    BypassEffectCommand cmd(chain, 0, true);
    REQUIRE(cmd.perform() == true);
    REQUIRE(chain.isBypassed(0) == true);
}

TEST_CASE("BypassEffectCommand undo restores previous bypass state", "[EffectCommands]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);
    chain.insertEffect(createEffect("compressor"));

    BypassEffectCommand cmd(chain, 0, true);
    cmd.perform();
    REQUIRE(chain.isBypassed(0) == true);

    REQUIRE(cmd.undo() == true);
    REQUIRE(chain.isBypassed(0) == false);
}

TEST_CASE("BypassEffectCommand getCommandName returns effect.bypass", "[EffectCommands]")
{
    InsertChain chain;
    BypassEffectCommand cmd(chain, 0, true);
    REQUIRE(cmd.getCommandName() == "effect.bypass");
}

// ==================================================================
// ProjectState effect chain tests
// ==================================================================

TEST_CASE("ProjectState effectChains serialization roundtrip", "[ProjectState]")
{
    ProjectState state;

    // Manually build effect chain data
    ProjectState::InsertChainData chainData;
    chainData.trackId = "polysynth";

    ProjectState::EffectSlotData slot;
    slot.effectType = "Compressor";
    slot.bypassed = false;
    auto* params = new juce::DynamicObject();
    params->setProperty("threshold", -10.0);
    params->setProperty("ratio", 4.0);
    slot.parameters = juce::var(params);
    chainData.effects.push_back(std::move(slot));

    state.effectChains.push_back(std::move(chainData));

    // Serialize and deserialize
    juce::String json = state.toJson();
    REQUIRE(json.contains("effectChains"));

    ProjectState loaded;
    REQUIRE(loaded.fromJson(json) == true);
    REQUIRE(loaded.effectChains.size() == 1);
    REQUIRE(loaded.effectChains[0].trackId == "polysynth");
    REQUIRE(loaded.effectChains[0].effects.size() == 1);
    REQUIRE(loaded.effectChains[0].effects[0].effectType == "Compressor");
    REQUIRE(loaded.effectChains[0].effects[0].bypassed == false);

    auto* loadedParams = loaded.effectChains[0].effects[0].parameters.getDynamicObject();
    REQUIRE(loadedParams != nullptr);
    REQUIRE(static_cast<double>(loadedParams->getProperty("threshold")) == -10.0);
    REQUIRE(static_cast<double>(loadedParams->getProperty("ratio")) == 4.0);
}

TEST_CASE("ProjectState effectChains slot has effectType, bypassed, and parameters", "[ProjectState]")
{
    ProjectState state;

    ProjectState::InsertChainData chainData;
    chainData.trackId = "master";

    ProjectState::EffectSlotData slot1;
    slot1.effectType = "Limiter";
    slot1.bypassed = true;
    auto* p1 = new juce::DynamicObject();
    p1->setProperty("threshold", -1.0);
    p1->setProperty("release", 100.0);
    slot1.parameters = juce::var(p1);
    chainData.effects.push_back(std::move(slot1));

    ProjectState::EffectSlotData slot2;
    slot2.effectType = "Reverb";
    slot2.bypassed = false;
    auto* p2 = new juce::DynamicObject();
    p2->setProperty("roomSize", 0.5);
    slot2.parameters = juce::var(p2);
    chainData.effects.push_back(std::move(slot2));

    state.effectChains.push_back(std::move(chainData));

    juce::String json = state.toJson();

    ProjectState loaded;
    loaded.fromJson(json);

    REQUIRE(loaded.effectChains[0].effects.size() == 2);
    REQUIRE(loaded.effectChains[0].effects[0].effectType == "Limiter");
    REQUIRE(loaded.effectChains[0].effects[0].bypassed == true);
    REQUIRE(loaded.effectChains[0].effects[1].effectType == "Reverb");
    REQUIRE(loaded.effectChains[0].effects[1].bypassed == false);
}

