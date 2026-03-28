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
