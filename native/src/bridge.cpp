#include <napi.h>
#include "bridge.h"
#include "calliope/engine.h"
#include <thread>

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
