#include <napi.h>
#include "bridge.h"
#include "calliope/engine.h"
#include "calliope/command_dispatcher.h"
#include "calliope/commands/transport_commands.h"
#include "calliope/commands/parameter_commands.h"
#include "calliope/project_state.h"
#include "calliope/parameter_registry.h"
#include <thread>
#include <memory>

Napi::Value GetEngineInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetEngineInfo",
        0,  // unlimited queue
        1   // initial thread count
    );

    std::thread([deferred, tsfn]() {
        auto juceVersion = calliope::Engine::getJuceVersion();
        auto devices = calliope::Engine::getAudioDevices();

        tsfn.BlockingCall([deferred, juceVersion, devices](
            Napi::Env env, Napi::Function) {
            auto result = Napi::Object::New(env);
            result.Set("juceVersion", Napi::String::New(env, juceVersion));

            auto deviceArray = Napi::Array::New(env, devices.size());
            for (size_t i = 0; i < devices.size(); i++) {
                deviceArray.Set(i, Napi::String::New(env, devices[i]));
            }
            result.Set("audioDevices", deviceArray);

            deferred.Resolve(result);
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value StartTestTone(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    double frequency = 440.0;
    if (info.Length() > 0 && info[0].IsNumber()) {
        frequency = info[0].As<Napi::Number>().DoubleValue();
    }

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "StartTestTone",
        0,
        1
    );

    std::thread([deferred, tsfn, frequency]() {
        bool success = calliope::Engine::startTestTone(frequency);

        tsfn.BlockingCall([deferred, success](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::Boolean::New(env, success));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value StopTestTone(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "StopTestTone",
        0,
        1
    );

    std::thread([deferred, tsfn]() {
        calliope::Engine::stopTestTone();

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

// ============================================================================
// Phase 2 — Engine lifecycle
// ============================================================================

Napi::Value InitialiseEngine(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    double sampleRate = 44100.0;
    int bufferSize = 512;
    if (info.Length() > 0 && info[0].IsNumber()) {
        sampleRate = info[0].As<Napi::Number>().DoubleValue();
    }
    if (info.Length() > 1 && info[1].IsNumber()) {
        bufferSize = info[1].As<Napi::Number>().Int32Value();
    }

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "InitialiseEngine",
        0, 1
    );

    std::thread([deferred, tsfn, sampleRate, bufferSize]() {
        bool success = calliope::Engine::getInstance().initialise(sampleRate, bufferSize);

        tsfn.BlockingCall([deferred, success](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::Boolean::New(env, success));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value ShutdownEngine(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "ShutdownEngine",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        calliope::Engine::getInstance().shutdown();

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

// ============================================================================
// Phase 2 — Transport
// ============================================================================

Napi::Value TransportPlay(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "TransportPlay",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        calliope::Engine::getInstance().transportPlay();

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value TransportStop(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "TransportStop",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        calliope::Engine::getInstance().transportStop();

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value TransportPause(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "TransportPause",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        calliope::Engine::getInstance().transportPause();

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value SetBpm(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected bpm as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    double bpm = info[0].As<Napi::Number>().DoubleValue();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "SetBpm",
        0, 1
    );

    std::thread([deferred, tsfn, bpm]() {
        calliope::Engine::getInstance().setBpm(bpm);

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value SetTimeSignature(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected numerator and denominator as numbers").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int numerator = info[0].As<Napi::Number>().Int32Value();
    int denominator = info[1].As<Napi::Number>().Int32Value();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "SetTimeSignature",
        0, 1
    );

    std::thread([deferred, tsfn, numerator, denominator]() {
        calliope::Engine::getInstance().setTimeSignature(numerator, denominator);

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value SetLoopRegion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsBoolean()) {
        Napi::TypeError::New(env, "Expected startBeat, endBeat (numbers) and enabled (boolean)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    double startBeat = info[0].As<Napi::Number>().DoubleValue();
    double endBeat = info[1].As<Napi::Number>().DoubleValue();
    bool enabled = info[2].As<Napi::Boolean>().Value();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "SetLoopRegion",
        0, 1
    );

    std::thread([deferred, tsfn, startBeat, endBeat, enabled]() {
        calliope::Engine::getInstance().setLoopRegion(startBeat, endBeat, enabled);

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

// ============================================================================
// Phase 2 — Config
// ============================================================================

Napi::Value SetBufferSize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected bufferSize as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int bufferSize = info[0].As<Napi::Number>().Int32Value();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "SetBufferSize",
        0, 1
    );

    std::thread([deferred, tsfn, bufferSize]() {
        bool success = calliope::Engine::getInstance().setBufferSize(bufferSize);

        tsfn.BlockingCall([deferred, success](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::Boolean::New(env, success));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

// ============================================================================
// Phase 2 — Metronome
// ============================================================================

Napi::Value SetMetronomeEnabled(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Expected enabled as boolean").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    bool enabled = info[0].As<Napi::Boolean>().Value();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "SetMetronomeEnabled",
        0, 1
    );

    std::thread([deferred, tsfn, enabled]() {
        calliope::Engine::getInstance().setMetronomeEnabled(enabled);

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value SetMetronomeVolume(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected volume as number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    float volume = info[0].As<Napi::Number>().FloatValue();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "SetMetronomeVolume",
        0, 1
    );

    std::thread([deferred, tsfn, volume]() {
        calliope::Engine::getInstance().setMetronomeVolume(volume);

        tsfn.BlockingCall([deferred](Napi::Env env, Napi::Function) {
            deferred.Resolve(env.Undefined());
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

// ============================================================================
// Phase 2 — State queries
// ============================================================================

Napi::Value GetTransportState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetTransportState",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        auto state = calliope::Engine::getInstance().getTransportState();

        tsfn.BlockingCall([deferred, state](Napi::Env env, Napi::Function) {
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

            deferred.Resolve(result);
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value GetAudioConfig(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetAudioConfig",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        auto config = calliope::Engine::getInstance().getAudioConfig();

        tsfn.BlockingCall([deferred, config](Napi::Env env, Napi::Function) {
            auto result = Napi::Object::New(env);
            result.Set("sampleRate", Napi::Number::New(env, config.sampleRate));
            result.Set("bufferSize", Napi::Number::New(env, config.bufferSize));
            result.Set("initialised", Napi::Boolean::New(env, config.initialised));

            deferred.Resolve(result);
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

// ============================================================================
// Phase 3 — Command dispatch
// ============================================================================

// Persistent event subscription state
static Napi::ThreadSafeFunction eventTsfn;
static bool eventSubscribed = false;

class BridgeListener : public calliope::CommandDispatcher::Listener {
public:
    explicit BridgeListener(Napi::ThreadSafeFunction tsfn) : tsfn_(tsfn) {}

    void commandExecuted(const calliope::CommandEvent& event) override {
        auto name = event.commandName.toStdString();
        auto data = juce::JSON::toString(event.data).toStdString();
        bool isUndo = event.isUndo;
        tsfn_.BlockingCall([name, data, isUndo](Napi::Env env, Napi::Function callback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, "execute"));
            obj.Set("command", Napi::String::New(env, name));
            obj.Set("data", Napi::String::New(env, data));
            callback.Call({obj});
        });
    }

    void commandUndone(const calliope::CommandEvent& event) override {
        auto name = event.commandName.toStdString();
        auto data = juce::JSON::toString(event.data).toStdString();
        tsfn_.BlockingCall([name, data](Napi::Env env, Napi::Function callback) {
            auto obj = Napi::Object::New(env);
            obj.Set("type", Napi::String::New(env, "undo"));
            obj.Set("command", Napi::String::New(env, name));
            obj.Set("data", Napi::String::New(env, data));
            callback.Call({obj});
        });
    }

private:
    Napi::ThreadSafeFunction tsfn_;
};

static std::unique_ptr<BridgeListener> bridgeListener;

Napi::Value DispatchCommand(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected command object {command: string, params: object}").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto cmdObj = info[0].As<Napi::Object>();
    std::string command = cmdObj.Get("command").As<Napi::String>().Utf8Value();

    // Extract all parameters on the JS thread as plain C++ types
    double bpm = 0.0;
    int numerator = 4, denominator = 4;
    double startBeat = 0.0, endBeat = 0.0;
    bool enabled = false;
    float volume = 0.0f;
    std::string paramId;
    double paramValueDouble = 0.0;
    bool paramValueBool = false;
    std::string paramValueType = "double";

    if (cmdObj.Has("params") && cmdObj.Get("params").IsObject()) {
        auto params = cmdObj.Get("params").As<Napi::Object>();
        if (params.Has("bpm") && params.Get("bpm").IsNumber())
            bpm = params.Get("bpm").As<Napi::Number>().DoubleValue();
        if (params.Has("numerator") && params.Get("numerator").IsNumber())
            numerator = params.Get("numerator").As<Napi::Number>().Int32Value();
        if (params.Has("denominator") && params.Get("denominator").IsNumber())
            denominator = params.Get("denominator").As<Napi::Number>().Int32Value();
        if (params.Has("startBeat") && params.Get("startBeat").IsNumber())
            startBeat = params.Get("startBeat").As<Napi::Number>().DoubleValue();
        if (params.Has("endBeat") && params.Get("endBeat").IsNumber())
            endBeat = params.Get("endBeat").As<Napi::Number>().DoubleValue();
        if (params.Has("enabled") && params.Get("enabled").IsBoolean())
            enabled = params.Get("enabled").As<Napi::Boolean>().Value();
        if (params.Has("volume") && params.Get("volume").IsNumber())
            volume = params.Get("volume").As<Napi::Number>().FloatValue();
        if (params.Has("id") && params.Get("id").IsString())
            paramId = params.Get("id").As<Napi::String>().Utf8Value();
        if (params.Has("value")) {
            auto val = params.Get("value");
            if (val.IsNumber()) {
                paramValueDouble = val.As<Napi::Number>().DoubleValue();
                paramValueType = "double";
            } else if (val.IsBoolean()) {
                paramValueBool = val.As<Napi::Boolean>().Value();
                paramValueType = "bool";
            }
        }
    }

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "DispatchCommand",
        0, 1
    );

    std::thread([deferred, tsfn, command, bpm, numerator, denominator,
                 startBeat, endBeat, enabled, volume,
                 paramId, paramValueDouble, paramValueBool, paramValueType]() {
        auto& engine = calliope::Engine::getInstance();
        std::unique_ptr<calliope::Command> cmd;

        if (command == "transport.play") {
            cmd = std::make_unique<calliope::PlayCommand>(engine.getTransport());
        } else if (command == "transport.stop") {
            cmd = std::make_unique<calliope::StopCommand>(engine.getTransport());
        } else if (command == "transport.pause") {
            cmd = std::make_unique<calliope::PauseCommand>(engine.getTransport());
        } else if (command == "transport.setBpm") {
            cmd = std::make_unique<calliope::SetBpmCommand>(engine.getTransport(), bpm);
        } else if (command == "transport.setTimeSignature") {
            cmd = std::make_unique<calliope::SetTimeSignatureCommand>(engine.getTransport(), numerator, denominator);
        } else if (command == "transport.setLoopRegion") {
            cmd = std::make_unique<calliope::SetLoopRegionCommand>(engine.getTransport(), startBeat, endBeat, enabled);
        } else if (command == "metronome.setEnabled") {
            cmd = std::make_unique<calliope::SetMetronomeEnabledCommand>(engine.getAudioGraph().getMetronome(), enabled);
        } else if (command == "metronome.setVolume") {
            cmd = std::make_unique<calliope::SetMetronomeVolumeCommand>(engine.getAudioGraph().getMetronome(), volume);
        } else if (command == "master.setVolume") {
            cmd = std::make_unique<calliope::SetMasterVolumeCommand>(engine.getAudioGraph().getMasterBus(), volume);
        } else if (command == "parameter.set") {
            juce::var val;
            if (paramValueType == "bool") {
                val = paramValueBool;
            } else {
                val = paramValueDouble;
            }
            cmd = std::make_unique<calliope::SetParameterCommand>(
                engine.getParameterRegistry(), juce::String(paramId), val);
        }

        bool success = false;
        if (cmd) {
            success = engine.getCommandDispatcher().dispatch(std::move(cmd));
        }

        tsfn.BlockingCall([deferred, success](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::Boolean::New(env, success));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value CommandUndo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "CommandUndo",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        bool success = calliope::Engine::getInstance().getCommandDispatcher().undo();

        tsfn.BlockingCall([deferred, success](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::Boolean::New(env, success));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value CommandRedo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "CommandRedo",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        bool success = calliope::Engine::getInstance().getCommandDispatcher().redo();

        tsfn.BlockingCall([deferred, success](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::Boolean::New(env, success));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value GetProjectState(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetProjectState",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        auto state = calliope::Engine::getInstance().getProjectState();
        auto jsonStr = state.toJson().toStdString();

        tsfn.BlockingCall([deferred, jsonStr](Napi::Env env, Napi::Function) {
            deferred.Resolve(Napi::String::New(env, jsonStr));
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value GetParameterIds(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    auto deferred = Napi::Promise::Deferred::New(env);

    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetParameterIds",
        0, 1
    );

    std::thread([deferred, tsfn]() {
        auto ids = calliope::Engine::getInstance().getParameterRegistry().getAllParameterIds();

        // Convert to std::vector<std::string> for thread-safe capture
        std::vector<std::string> idStrs;
        idStrs.reserve(ids.size());
        for (const auto& id : ids) {
            idStrs.push_back(id.toStdString());
        }

        tsfn.BlockingCall([deferred, idStrs](Napi::Env env, Napi::Function) {
            auto arr = Napi::Array::New(env, idStrs.size());
            for (size_t i = 0; i < idStrs.size(); i++) {
                arr.Set(i, Napi::String::New(env, idStrs[i]));
            }
            deferred.Resolve(arr);
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}

Napi::Value SubscribeToEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Unsubscribe previous listener if any
    if (eventSubscribed && bridgeListener) {
        calliope::Engine::getInstance().getCommandDispatcher().removeListener(bridgeListener.get());
        bridgeListener.reset();
        eventTsfn.Release();
        eventSubscribed = false;
    }

    auto callback = info[0].As<Napi::Function>();

    // Create a persistent TSFN with the user's callback
    eventTsfn = Napi::ThreadSafeFunction::New(
        env,
        callback,
        "CommandEventSubscription",
        0,  // unlimited queue
        1   // initial thread count
    );

    bridgeListener = std::make_unique<BridgeListener>(eventTsfn);
    calliope::Engine::getInstance().getCommandDispatcher().addListener(bridgeListener.get());
    eventSubscribed = true;

    return env.Undefined();
}

Napi::Value UnsubscribeFromEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (eventSubscribed && bridgeListener) {
        calliope::Engine::getInstance().getCommandDispatcher().removeListener(bridgeListener.get());
        bridgeListener.reset();
        eventTsfn.Release();
        eventSubscribed = false;
    }

    return env.Undefined();
}
