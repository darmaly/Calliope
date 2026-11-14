// Node-API surface of the audio engine.
//
// Threading model
// ---------------
// Almost every call here is cheap (an atomic store, a graph edit, a snapshot
// publish) and runs synchronously on the JavaScript thread, returning an
// already-resolved Promise. That is deliberate: the previous version started a
// detached thread per call, so two calls made in order (drag a fader, move a
// note twice) could be applied out of order, and concurrent calls raced on the
// engine's non-audio state. Running them on one thread makes the order of
// effects the order of calls.
//
// Only calls that can block for a noticeable time run on a worker thread:
// opening and closing the audio device, and offline export.

#include <napi.h>
#include "bridge.h"
#include "calliope/engine.h"
#include "calliope/command_dispatcher.h"
#include "calliope/commands/transport_commands.h"
#include "calliope/commands/parameter_commands.h"
#include "calliope/commands/instrument_commands.h"
#include "calliope/commands/effect_commands.h"
#include "calliope/project_state.h"
#include "calliope/clip_scheduler.h"
#include "calliope/parameter_registry.h"
#include "calliope/audio_exporter.h"
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

using calliope::Engine;

namespace {

// Set while an offline export owns the graph. The UI is modal during export;
// this makes the engine refuse edits even if a call slips through.
std::atomic<bool> exportInProgress{false};

bool engineReady()
{
    return Engine::getInstance().isInitialised() && !exportInProgress.load();
}

juce::String toJuce(const std::string& utf8)
{
    return juce::String::fromUTF8(utf8.c_str());
}

Napi::Value resolved(Napi::Env env, Napi::Value value)
{
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Resolve(value);
    return deferred.Promise();
}

Napi::Value resolvedBool(Napi::Env env, bool value)
{
    return resolved(env, Napi::Boolean::New(env, value));
}

// Runs `work` on its own thread, then resolves the returned Promise on the
// JavaScript thread with convert(env, result).
template <typename Result>
Napi::Value runAsync(Napi::Env env, const char* name,
                     std::function<Result()> work,
                     std::function<Napi::Value(Napi::Env, const Result&)> convert)
{
    auto deferred = Napi::Promise::Deferred::New(env);
    auto tsfn = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}), name, 0, 1);

    std::thread([deferred, tsfn, work = std::move(work), convert = std::move(convert)]() mutable {
        auto result = std::make_shared<Result>(work());
        tsfn.BlockingCall([deferred, result, convert](Napi::Env env, Napi::Function) {
            deferred.Resolve(convert(env, *result));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value boolToJs(Napi::Env env, const bool& value)
{
    return Napi::Boolean::New(env, value);
}

// --- Typed, defaulted property access ---

std::string getString(const Napi::Object& obj, const char* key, const std::string& fallback = {})
{
    auto value = obj.Get(key);
    return value.IsString() ? value.As<Napi::String>().Utf8Value() : fallback;
}

double getNumber(const Napi::Object& obj, const char* key, double fallback = 0.0)
{
    auto value = obj.Get(key);
    return value.IsNumber() ? value.As<Napi::Number>().DoubleValue() : fallback;
}

bool getBool(const Napi::Object& obj, const char* key, bool fallback = false)
{
    auto value = obj.Get(key);
    return value.IsBoolean() ? value.As<Napi::Boolean>().Value() : fallback;
}

// Velocities arrive as MIDI values (1-127) from the editor; the engine works
// in 0..1. Values already in 0..1 are taken as they are.
float toUnitVelocity(double velocity)
{
    if (velocity > 1.0)
        velocity /= 127.0;
    return static_cast<float>(juce::jlimit(0.0, 1.0, velocity));
}

Napi::Value varToJs(Napi::Env env, const juce::var& value)
{
    if (value.isBool())   return Napi::Boolean::New(env, static_cast<bool>(value));
    if (value.isInt() || value.isInt64() || value.isDouble())
        return Napi::Number::New(env, static_cast<double>(value));
    if (value.isString()) return Napi::String::New(env, value.toString().toStdString());
    return env.Null();
}

} // namespace

// ============================================================================
// Engine information and test tone
// ============================================================================

Napi::Value GetEngineInfo(const Napi::CallbackInfo& info)
{
    struct Result {
        std::string juceVersion;
        std::vector<std::string> devices;
    };

    return runAsync<Result>(
        info.Env(), "GetEngineInfo",
        []() { return Result{Engine::getJuceVersion(), Engine::getAudioDevices()}; },
        [](Napi::Env env, const Result& r) -> Napi::Value {
            auto result = Napi::Object::New(env);
            result.Set("juceVersion", Napi::String::New(env, r.juceVersion));
            auto devices = Napi::Array::New(env, r.devices.size());
            for (size_t i = 0; i < r.devices.size(); ++i)
                devices.Set(static_cast<uint32_t>(i), Napi::String::New(env, r.devices[i]));
            result.Set("audioDevices", devices);
            return result;
        });
}

Napi::Value StartTestTone(const Napi::CallbackInfo& info)
{
    double frequency = 440.0;
    if (info.Length() > 0 && info[0].IsNumber())
        frequency = info[0].As<Napi::Number>().DoubleValue();

    return runAsync<bool>(
        info.Env(), "StartTestTone",
        [frequency]() { return Engine::startTestTone(frequency); },
        boolToJs);
}

Napi::Value StopTestTone(const Napi::CallbackInfo& info)
{
    return runAsync<bool>(
        info.Env(), "StopTestTone",
        []() { Engine::stopTestTone(); return true; },
        [](Napi::Env env, const bool&) -> Napi::Value { return env.Undefined(); });
}

// ============================================================================
// Lifecycle
// ============================================================================

Napi::Value InitialiseEngine(const Napi::CallbackInfo& info)
{
    double sampleRate = 44100.0;
    int bufferSize = 512;
    if (info.Length() > 0 && info[0].IsNumber())
        sampleRate = info[0].As<Napi::Number>().DoubleValue();
    if (info.Length() > 1 && info[1].IsNumber())
        bufferSize = info[1].As<Napi::Number>().Int32Value();

    return runAsync<bool>(
        info.Env(), "InitialiseEngine",
        [sampleRate, bufferSize]() { return Engine::getInstance().initialise(sampleRate, bufferSize); },
        boolToJs);
}

Napi::Value ShutdownEngine(const Napi::CallbackInfo& info)
{
    if (exportInProgress.load())
        return resolved(info.Env(), info.Env().Undefined());

    return runAsync<bool>(
        info.Env(), "ShutdownEngine",
        []() { Engine::getInstance().shutdown(); return true; },
        [](Napi::Env env, const bool&) -> Napi::Value { return env.Undefined(); });
}

Napi::Value SetBufferSize(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected bufferSize as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!engineReady())
        return resolvedBool(env, false);

    const int bufferSize = info[0].As<Napi::Number>().Int32Value();
    return runAsync<bool>(
        env, "SetBufferSize",
        [bufferSize]() { return Engine::getInstance().setBufferSize(bufferSize); },
        boolToJs);
}

// ============================================================================
// Transport and metronome
// ============================================================================

Napi::Value TransportPlay(const Napi::CallbackInfo& info)
{
    if (engineReady()) Engine::getInstance().transportPlay();
    return resolved(info.Env(), info.Env().Undefined());
}

Napi::Value TransportStop(const Napi::CallbackInfo& info)
{
    if (engineReady()) Engine::getInstance().transportStop();
    return resolved(info.Env(), info.Env().Undefined());
}

Napi::Value TransportPause(const Napi::CallbackInfo& info)
{
    if (engineReady()) Engine::getInstance().transportPause();
    return resolved(info.Env(), info.Env().Undefined());
}

Napi::Value SetBpm(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected BPM as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (engineReady())
        Engine::getInstance().setBpm(info[0].As<Napi::Number>().DoubleValue());
    return resolved(env, env.Undefined());
}

Napi::Value SetTimeSignature(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected numerator and denominator as numbers").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (engineReady())
        Engine::getInstance().setTimeSignature(info[0].As<Napi::Number>().Int32Value(),
                                               info[1].As<Napi::Number>().Int32Value());
    return resolved(env, env.Undefined());
}

Napi::Value SetLoopRegion(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsBoolean()) {
        Napi::TypeError::New(env, "Expected startBeat, endBeat, enabled").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (engineReady())
        Engine::getInstance().setLoopRegion(info[0].As<Napi::Number>().DoubleValue(),
                                            info[1].As<Napi::Number>().DoubleValue(),
                                            info[2].As<Napi::Boolean>().Value());
    return resolved(env, env.Undefined());
}

Napi::Value SetMetronomeEnabled(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Expected enabled as boolean").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (engineReady())
        Engine::getInstance().setMetronomeEnabled(info[0].As<Napi::Boolean>().Value());
    return resolved(env, env.Undefined());
}

Napi::Value SetMetronomeVolume(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected volume as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (engineReady())
        Engine::getInstance().setMetronomeVolume(info[0].As<Napi::Number>().FloatValue());
    return resolved(env, env.Undefined());
}

// ============================================================================
// State queries
// ============================================================================

Napi::Value GetTransportState(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto state = Engine::getInstance().getTransportState();

    auto result = Napi::Object::New(env);
    result.Set("state", Napi::String::New(env, state.state));
    result.Set("bpm", Napi::Number::New(env, state.bpm));
    result.Set("timeSigNumerator", Napi::Number::New(env, state.timeSigNumerator));
    result.Set("timeSigDenominator", Napi::Number::New(env, state.timeSigDenominator));
    result.Set("samplePosition", Napi::Number::New(env, static_cast<double>(state.samplePosition)));
    result.Set("ppqPosition", Napi::Number::New(env, state.ppqPosition));
    result.Set("looping", Napi::Boolean::New(env, state.looping));
    result.Set("loopStartBeat", Napi::Number::New(env, state.loopStartBeat));
    result.Set("loopEndBeat", Napi::Number::New(env, state.loopEndBeat));
    return resolved(env, result);
}

Napi::Value GetAudioConfig(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto config = Engine::getInstance().getAudioConfig();

    auto result = Napi::Object::New(env);
    result.Set("sampleRate", Napi::Number::New(env, config.sampleRate));
    result.Set("bufferSize", Napi::Number::New(env, config.bufferSize));
    result.Set("initialised", Napi::Boolean::New(env, config.initialised));
    return resolved(env, result);
}

Napi::Value GetMeterLevels(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto levels = Engine::getInstance().getMeterLevels();

    auto toObject = [&env](const calliope::AudioGraph::AllMeterLevels::TrackMeter& m) {
        auto obj = Napi::Object::New(env);
        obj.Set("rmsLeft", Napi::Number::New(env, m.rmsLeft));
        obj.Set("rmsRight", Napi::Number::New(env, m.rmsRight));
        obj.Set("peakLeft", Napi::Number::New(env, m.peakLeft));
        obj.Set("peakRight", Napi::Number::New(env, m.peakRight));
        return obj;
    };

    auto result = Napi::Object::New(env);
    for (const auto& track : levels.tracks)
        result.Set(track.trackId, toObject(track));
    result.Set("master", toObject(levels.master));
    return resolved(env, result);
}

// ============================================================================
// Tracks
// ============================================================================

Napi::Value AddTrack(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected {trackId: string, instrument: string}").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto obj = info[0].As<Napi::Object>();
    const auto trackId = getString(obj, "trackId");
    const auto instrument = getString(obj, "instrument");

    if (!engineReady())
        return resolvedBool(env, false);
    return resolvedBool(env, Engine::getInstance().addTrack(trackId, instrument));
}

Napi::Value RemoveTrack(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected trackId as string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!engineReady())
        return resolvedBool(env, false);
    return resolvedBool(env, Engine::getInstance().removeTrack(info[0].As<Napi::String>().Utf8Value()));
}

Napi::Value ResetProject(const Napi::CallbackInfo& info)
{
    if (!engineReady())
        return resolvedBool(info.Env(), false);
    Engine::getInstance().resetProject();
    return resolvedBool(info.Env(), true);
}

// ============================================================================
// Clips
// ============================================================================

static bool parseClipObject(const Napi::Object& obj, calliope::MidiClipData& clip)
{
    clip.clipId = getString(obj, "clipId");
    clip.trackId = getString(obj, "trackId");
    clip.startBeat = getNumber(obj, "startBeat");
    clip.lengthBeats = getNumber(obj, "lengthBeats");
    if (clip.clipId.empty() || clip.trackId.empty())
        return false;

    auto notesValue = obj.Get("notes");
    if (notesValue.IsArray()) {
        auto notes = notesValue.As<Napi::Array>();
        clip.notes.reserve(notes.Length());
        for (uint32_t i = 0; i < notes.Length(); ++i) {
            auto noteValue = notes.Get(i);
            if (!noteValue.IsObject()) continue;
            auto noteObj = noteValue.As<Napi::Object>();

            calliope::MidiNoteData note;
            note.pitch = static_cast<int>(getNumber(noteObj, "pitch", 60.0));
            note.startBeat = getNumber(noteObj, "startBeat");
            note.lengthBeats = getNumber(noteObj, "lengthBeats");
            note.velocity = toUnitVelocity(getNumber(noteObj, "velocity", 100.0));
            clip.notes.push_back(note);
        }
    }
    return true;
}

static Napi::Value setClip(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected clip object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    calliope::MidiClipData clip;
    if (!parseClipObject(info[0].As<Napi::Object>(), clip) || !engineReady())
        return resolvedBool(env, false);

    Engine::getInstance().getClipScheduler().updateClip(clip);
    return resolvedBool(env, true);
}

Napi::Value AddClip(const Napi::CallbackInfo& info)    { return setClip(info); }
Napi::Value UpdateClip(const Napi::CallbackInfo& info) { return setClip(info); }

Napi::Value RemoveClip(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected clipId as string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!engineReady())
        return resolvedBool(env, false);
    Engine::getInstance().getClipScheduler().removeClip(info[0].As<Napi::String>().Utf8Value());
    return resolvedBool(env, true);
}

Napi::Value ClearClips(const Napi::CallbackInfo& info)
{
    if (!engineReady())
        return resolvedBool(info.Env(), false);
    Engine::getInstance().getClipScheduler().clearClips();
    return resolvedBool(info.Env(), true);
}

// ============================================================================
// Command dispatch
// ============================================================================

// Persistent event subscription state
static Napi::ThreadSafeFunction eventTsfn;
static bool eventSubscribed = false;

class BridgeListener : public calliope::CommandDispatcher::Listener {
public:
    explicit BridgeListener(Napi::ThreadSafeFunction tsfn) : tsfn_(tsfn) {}

    void commandExecuted(const calliope::CommandEvent& event) override { emit("execute", event); }
    void commandUndone(const calliope::CommandEvent& event) override   { emit("undo", event); }

private:
    void emit(const char* type, const calliope::CommandEvent& event)
    {
        std::string typeName = type;
        auto name = event.commandName.toStdString();
        auto data = juce::JSON::toString(event.data).toStdString();
        tsfn_.NonBlockingCall([typeName, name, data](Napi::Env env, Napi::Function callback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, typeName));
            obj.Set("command", Napi::String::New(env, name));
            obj.Set("data", Napi::String::New(env, data));
            callback.Call({obj});
        });
    }

    Napi::ThreadSafeFunction tsfn_;
};

static std::unique_ptr<BridgeListener> bridgeListener;

// Supported commands:
//   transport.play | transport.stop | transport.pause
//   transport.setBpm            {bpm}
//   transport.setTimeSignature  {numerator, denominator}
//   transport.setLoopRegion     {startBeat, endBeat, enabled}
//   metronome.setEnabled        {enabled}
//   metronome.setVolume         {volume}
//   master.setVolume            {volume}
//   parameter.set               {id, value}
//   instrument.noteOn           {trackId, note, velocity}
//   instrument.noteOff          {trackId, note}
//   drumMachine.loadSample      {trackId, padIndex, filePath}
//   effect.insert               {trackId, effectType, position?}
//   effect.remove               {trackId, position}
//   effect.reorder              {trackId, fromPosition, toPosition}
//   effect.bypass               {trackId, position, bypassed}
// trackId is a track's ID, or "master" for the effect commands.
Napi::Value DispatchCommand(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected command object {command: string, params: object}").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto cmdObj = info[0].As<Napi::Object>();
    const std::string command = getString(cmdObj, "command");
    auto paramsValue = cmdObj.Get("params");
    auto params = paramsValue.IsObject() ? paramsValue.As<Napi::Object>() : Napi::Object::New(env);

    if (!engineReady())
        return resolvedBool(env, false);

    auto& engine = Engine::getInstance();
    auto& graph = engine.getAudioGraph();
    const auto trackId = getString(params, "trackId", getString(params, "instrument"));
    std::unique_ptr<calliope::Command> cmd;

    if (command == "transport.play") {
        cmd = std::make_unique<calliope::PlayCommand>(engine.getTransport());
    } else if (command == "transport.stop") {
        cmd = std::make_unique<calliope::StopCommand>(engine.getTransport());
    } else if (command == "transport.pause") {
        cmd = std::make_unique<calliope::PauseCommand>(engine.getTransport());
    } else if (command == "transport.setBpm") {
        cmd = std::make_unique<calliope::SetBpmCommand>(
            engine.getTransport(), juce::jlimit(20.0, 999.0, getNumber(params, "bpm", 120.0)));
    } else if (command == "transport.setTimeSignature") {
        cmd = std::make_unique<calliope::SetTimeSignatureCommand>(
            engine.getTransport(),
            juce::jlimit(1, 32, static_cast<int>(getNumber(params, "numerator", 4))),
            juce::jlimit(1, 32, static_cast<int>(getNumber(params, "denominator", 4))));
    } else if (command == "transport.setLoopRegion") {
        const double startBeat = getNumber(params, "startBeat");
        const double endBeat = getNumber(params, "endBeat");
        cmd = std::make_unique<calliope::SetLoopRegionCommand>(
            engine.getTransport(), startBeat, endBeat,
            getBool(params, "enabled") && endBeat > startBeat);
    } else if (command == "metronome.setEnabled") {
        cmd = std::make_unique<calliope::SetMetronomeEnabledCommand>(
            graph.getMetronome(), getBool(params, "enabled"));
    } else if (command == "metronome.setVolume") {
        cmd = std::make_unique<calliope::SetMetronomeVolumeCommand>(
            graph.getMetronome(), static_cast<float>(juce::jlimit(0.0, 1.0, getNumber(params, "volume"))));
    } else if (command == "master.setVolume") {
        cmd = std::make_unique<calliope::SetMasterVolumeCommand>(
            graph.getMasterBus(), static_cast<float>(juce::jlimit(0.0, 2.0, getNumber(params, "volume", 1.0))));
    } else if (command == "parameter.set") {
        auto value = params.Get("value");
        juce::var var;
        if (value.IsBoolean())      var = value.As<Napi::Boolean>().Value();
        else if (value.IsNumber())  var = value.As<Napi::Number>().DoubleValue();
        if (!var.isVoid())
            cmd = std::make_unique<calliope::SetParameterCommand>(
                engine.getParameterRegistry(), toJuce(getString(params, "id")), var);
    } else if (command == "instrument.noteOn") {
        if (auto* instrument = graph.getTrackInstrument(trackId))
            cmd = std::make_unique<calliope::NoteOnCommand>(
                *instrument, static_cast<int>(getNumber(params, "note", 60)),
                toUnitVelocity(getNumber(params, "velocity", 0.8)));
    } else if (command == "instrument.noteOff") {
        if (auto* instrument = graph.getTrackInstrument(trackId))
            cmd = std::make_unique<calliope::NoteOffCommand>(
                *instrument, static_cast<int>(getNumber(params, "note", 60)));
    } else if (command == "drumMachine.loadSample") {
        if (auto* drums = dynamic_cast<calliope::DrumMachineProcessor*>(graph.getTrackInstrument(trackId)))
            cmd = std::make_unique<calliope::LoadSampleCommand>(
                *drums, static_cast<int>(getNumber(params, "padIndex")), getString(params, "filePath"));
    } else if (command.rfind("effect.", 0) == 0) {
        // An unknown track has no chain. (It used to fall through to the master chain.)
        const auto chainId = toJuce(trackId);
        if (auto* chain = engine.findInsertChain(chainId)) {
            const int position = static_cast<int>(getNumber(params, "position", -1));
            if (command == "effect.insert") {
                cmd = std::make_unique<calliope::InsertEffectCommand>(
                    *chain, engine, chainId, toJuce(getString(params, "effectType")), position);
            } else if (command == "effect.remove") {
                cmd = std::make_unique<calliope::RemoveEffectCommand>(*chain, engine, chainId, position);
            } else if (command == "effect.reorder") {
                cmd = std::make_unique<calliope::ReorderEffectCommand>(
                    *chain, engine, chainId,
                    static_cast<int>(getNumber(params, "fromPosition")),
                    static_cast<int>(getNumber(params, "toPosition")));
            } else if (command == "effect.bypass") {
                if (position >= 0 && position < chain->getNumEffects())
                    cmd = std::make_unique<calliope::BypassEffectCommand>(
                        *chain, position, getBool(params, "bypassed"));
            }
        }
    }

    const bool success = cmd != nullptr && engine.getCommandDispatcher().dispatch(std::move(cmd));
    return resolvedBool(env, success);
}

Napi::Value CommandUndo(const Napi::CallbackInfo& info)
{
    if (!engineReady())
        return resolvedBool(info.Env(), false);
    return resolvedBool(info.Env(), Engine::getInstance().getCommandDispatcher().undo());
}

Napi::Value CommandRedo(const Napi::CallbackInfo& info)
{
    if (!engineReady())
        return resolvedBool(info.Env(), false);
    return resolvedBool(info.Env(), Engine::getInstance().getCommandDispatcher().redo());
}

Napi::Value GetProjectState(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto json = Engine::getInstance().getProjectState().toJson().toStdString();
    return resolved(env, Napi::String::New(env, json));
}

Napi::Value LoadProjectState(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected JSON string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!engineReady())
        return resolvedBool(env, false);

    calliope::ProjectState state;
    if (!state.fromJson(toJuce(info[0].As<Napi::String>().Utf8Value())))
        return resolvedBool(env, false);
    return resolvedBool(env, Engine::getInstance().applyProjectState(state));
}

Napi::Value GetParameterIds(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto ids = Engine::getInstance().getParameterRegistry().getAllParameterIds();

    auto result = Napi::Array::New(env, ids.size());
    for (size_t i = 0; i < ids.size(); ++i)
        result.Set(static_cast<uint32_t>(i), Napi::String::New(env, ids[i].toStdString()));
    return resolved(env, result);
}

// Every parameter with its type, range and current value: what a UI needs to
// draw a control, and what an agent needs to know what it may set.
Napi::Value DescribeParameters(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto& registry = Engine::getInstance().getParameterRegistry();
    auto ids = registry.getAllParameterIds();

    auto result = Napi::Array::New(env);
    uint32_t index = 0;
    for (const auto& id : ids) {
        const auto* def = registry.getParameter(id);
        if (def == nullptr) continue;

        auto obj = Napi::Object::New(env);
        obj.Set("id", Napi::String::New(env, id.toStdString()));
        obj.Set("type", Napi::String::New(env, def->type.toStdString()));
        obj.Set("min", varToJs(env, def->minValue));
        obj.Set("max", varToJs(env, def->maxValue));
        obj.Set("value", varToJs(env, def->getter()));
        result.Set(index++, obj);
    }
    return resolved(env, result);
}

Napi::Value SubscribeToEvents(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto& dispatcher = Engine::getInstance().getCommandDispatcher();
    if (eventSubscribed && bridgeListener) {
        dispatcher.removeListener(bridgeListener.get());
        bridgeListener.reset();
        eventTsfn.Release();
        eventSubscribed = false;
    }

    eventTsfn = Napi::ThreadSafeFunction::New(
        env, info[0].As<Napi::Function>(), "CommandEventSubscription", 0, 1);
    // Events must not keep the process alive on their own.
    eventTsfn.Unref(env);

    bridgeListener = std::make_unique<BridgeListener>(eventTsfn);
    dispatcher.addListener(bridgeListener.get());
    eventSubscribed = true;

    return env.Undefined();
}

Napi::Value UnsubscribeFromEvents(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (eventSubscribed && bridgeListener) {
        Engine::getInstance().getCommandDispatcher().removeListener(bridgeListener.get());
        bridgeListener.reset();
        eventTsfn.Release();
        eventSubscribed = false;
    }

    return env.Undefined();
}

// ============================================================================
// Export
// ============================================================================

namespace {

// Wraps an optional JS progress callback so a worker thread can report
// progress, at most once per percent.
struct ProgressReporter {
    Napi::ThreadSafeFunction tsfn;
    bool active = false;
    std::shared_ptr<std::atomic<int>> lastPercent = std::make_shared<std::atomic<int>>(-1);

    static ProgressReporter create(Napi::Env env, const Napi::Value& callback, const char* name)
    {
        ProgressReporter reporter;
        if (callback.IsFunction()) {
            reporter.tsfn = Napi::ThreadSafeFunction::New(env, callback.As<Napi::Function>(), name, 0, 1);
            reporter.active = true;
        }
        return reporter;
    }

    calliope::ProgressCallback callback() const
    {
        if (!active) return nullptr;
        auto tsfnCopy = tsfn;
        auto last = lastPercent;
        return [tsfnCopy, last](float fraction) {
            const int percent = static_cast<int>(fraction * 100.0f);
            if (last->exchange(percent) == percent) return;
            tsfnCopy.NonBlockingCall([fraction](Napi::Env env, Napi::Function fn) {
                fn.Call({Napi::Number::New(env, static_cast<double>(fraction))});
            });
        };
    }

    void release() { if (active) tsfn.Release(); }
};

} // namespace

Napi::Value GetExportFormats(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    auto result = Napi::Object::New(env);
    result.Set("wav16", Napi::Boolean::New(env, true));
    result.Set("wav24", Napi::Boolean::New(env, true));
    result.Set("flac", Napi::Boolean::New(env, true));
    result.Set("mp3", Napi::Boolean::New(env, calliope::AudioExporter::isFormatSupported(calliope::ExportFormat::MP3)));
    return result;
}

// exportAudio({outputPath, format: 'wav16'|'wav24'|'flac'|'mp3', mp3Bitrate?, totalBeats?}, onProgress?)
Napi::Value ExportAudio(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected export options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto obj = info[0].As<Napi::Object>();

    calliope::ExportOptions options;
    options.outputPath = getString(obj, "outputPath");
    options.mp3Bitrate = static_cast<int>(getNumber(obj, "mp3Bitrate", 192));
    options.totalBeats = getNumber(obj, "totalBeats", 0.0);

    const auto format = getString(obj, "format", "wav24");
    if (format == "wav16")      options.format = calliope::ExportFormat::WAV_16;
    else if (format == "wav24") options.format = calliope::ExportFormat::WAV_24;
    else if (format == "flac")  options.format = calliope::ExportFormat::FLAC;
    else if (format == "mp3")   options.format = calliope::ExportFormat::MP3;
    else return resolvedBool(env, false);

    if (!Engine::getInstance().isInitialised() || exportInProgress.exchange(true))
        return resolvedBool(env, false);

    auto reporter = ProgressReporter::create(env, info.Length() > 1 ? info[1] : env.Undefined(), "ExportAudioProgress");

    return runAsync<bool>(
        env, "ExportAudio",
        [options, reporter]() mutable {
            const bool ok = Engine::getInstance().getAudioExporter().exportMix(options, reporter.callback());
            reporter.release();
            exportInProgress.store(false);
            return ok;
        },
        boolToJs);
}

// exportStems({outputDir, totalBeats?, trackNames?: {[trackId]: string}}, onProgress?)
Napi::Value ExportStems(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected stem export options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto obj = info[0].As<Napi::Object>();

    calliope::StemExportOptions options;
    options.outputDir = getString(obj, "outputDir");
    options.totalBeats = getNumber(obj, "totalBeats", 0.0);

    auto namesValue = obj.Get("trackNames");
    if (namesValue.IsObject()) {
        auto names = namesValue.As<Napi::Object>();
        auto keys = names.GetPropertyNames();
        for (uint32_t i = 0; i < keys.Length(); ++i) {
            auto key = keys.Get(i);
            auto value = names.Get(key);
            if (key.IsString() && value.IsString())
                options.trackNames[key.As<Napi::String>().Utf8Value()] = value.As<Napi::String>().Utf8Value();
        }
    }

    if (!Engine::getInstance().isInitialised() || exportInProgress.exchange(true))
        return resolvedBool(env, false);

    auto reporter = ProgressReporter::create(env, info.Length() > 1 ? info[1] : env.Undefined(), "ExportStemsProgress");

    return runAsync<bool>(
        env, "ExportStems",
        [options, reporter]() mutable {
            const bool ok = Engine::getInstance().getAudioExporter().exportStems(options, reporter.callback());
            reporter.release();
            exportInProgress.store(false);
            return ok;
        },
        boolToJs);
}
