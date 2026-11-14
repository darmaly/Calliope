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

    ProjectState::InsertChainData chainData;
    chainData.trackId = "lead";
    chainData.effects.push_back({"compressor", false});
    state.effectChains.push_back(std::move(chainData));

    // Effect settings are ordinary parameters, keyed by track and slot.
    auto* params = new juce::DynamicObject();
    params->setProperty("effects.lead.0.threshold", -10.0);
    params->setProperty("effects.lead.0.ratio", 4.0);
    state.parameters = juce::var(params);

    juce::String json = state.toJson();
    REQUIRE(json.contains("effectChains"));

    ProjectState loaded;
    REQUIRE(loaded.fromJson(json) == true);
    REQUIRE(loaded.effectChains.size() == 1);
    REQUIRE(loaded.effectChains[0].trackId == "lead");
    REQUIRE(loaded.effectChains[0].effects.size() == 1);
    REQUIRE(loaded.effectChains[0].effects[0].effectType == "compressor");
    REQUIRE(loaded.effectChains[0].effects[0].bypassed == false);

    auto* loadedParams = loaded.parameters.getDynamicObject();
    REQUIRE(loadedParams != nullptr);
    REQUIRE(static_cast<double>(loadedParams->getProperty("effects.lead.0.threshold")) == -10.0);
    REQUIRE(static_cast<double>(loadedParams->getProperty("effects.lead.0.ratio")) == 4.0);
}

TEST_CASE("ProjectState effectChains keep slot order and bypass state", "[ProjectState]")
{
    ProjectState state;

    ProjectState::InsertChainData chainData;
    chainData.trackId = "master";
    chainData.effects.push_back({"limiter", true});
    chainData.effects.push_back({"reverb", false});
    state.effectChains.push_back(std::move(chainData));

    ProjectState loaded;
    REQUIRE(loaded.fromJson(state.toJson()));

    REQUIRE(loaded.effectChains[0].effects.size() == 2);
    REQUIRE(loaded.effectChains[0].effects[0].effectType == "limiter");
    REQUIRE(loaded.effectChains[0].effects[0].bypassed == true);
    REQUIRE(loaded.effectChains[0].effects[1].effectType == "reverb");
    REQUIRE(loaded.effectChains[0].effects[1].bypassed == false);
}

TEST_CASE("effectTypeForProcessor names every effect createEffect can build", "[EffectCommands]")
{
    for (const char* type : {"eq", "compressor", "reverb", "delay", "limiter"}) {
        auto effect = createEffect(type);
        REQUIRE(effect != nullptr);
        REQUIRE(effectTypeForProcessor(*effect) == juce::String(type));
    }
}

TEST_CASE("ProjectState round-trips tracks, clips and notes", "[ProjectState]")
{
    ProjectState state;
    state.tracks.push_back({"t1", "polysynth"});
    state.tracks.push_back({"t2", "drumMachine"});

    MidiClipData clip;
    clip.clipId = "c1";
    clip.trackId = "t2";
    clip.startBeat = 8.0;
    clip.lengthBeats = 4.0;
    clip.notes.push_back({36, 0.0, 0.25, 1.0f});
    clip.notes.push_back({42, 0.5, 0.25, 0.5f});
    state.clips.push_back(clip);

    ProjectState loaded;
    REQUIRE(loaded.fromJson(state.toJson()));

    REQUIRE(loaded.formatVersion == ProjectState::kFormatVersion);
    REQUIRE(loaded.tracks.size() == 2);
    REQUIRE(loaded.tracks[1].id == "t2");
    REQUIRE(loaded.tracks[1].instrument == "drumMachine");
    REQUIRE(loaded.clips.size() == 1);
    REQUIRE(loaded.clips[0].trackId == "t2");
    REQUIRE(loaded.clips[0].startBeat == 8.0);
    REQUIRE(loaded.clips[0].notes.size() == 2);
    REQUIRE(loaded.clips[0].notes[1].pitch == 42);
    REQUIRE(loaded.clips[0].notes[1].velocity == 0.5f);
}

TEST_CASE("ProjectState fromJson rejects input that is not a JSON object", "[ProjectState]")
{
    ProjectState state;
    REQUIRE_FALSE(state.fromJson("not json"));
    REQUIRE_FALSE(state.fromJson("[1, 2, 3]"));
    REQUIRE_FALSE(state.fromJson(""));
}
