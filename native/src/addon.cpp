#include <napi.h>
#include "bridge.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Phase 1
    exports.Set("getEngineInfo",
        Napi::Function::New(env, GetEngineInfo));
    exports.Set("startTestTone",
        Napi::Function::New(env, StartTestTone));
    exports.Set("stopTestTone",
        Napi::Function::New(env, StopTestTone));

    // Phase 2 — Engine lifecycle
    exports.Set("initialiseEngine",
        Napi::Function::New(env, InitialiseEngine));
    exports.Set("shutdownEngine",
        Napi::Function::New(env, ShutdownEngine));

    // Phase 2 — Transport
    exports.Set("transportPlay",
        Napi::Function::New(env, TransportPlay));
    exports.Set("transportStop",
        Napi::Function::New(env, TransportStop));
    exports.Set("transportPause",
        Napi::Function::New(env, TransportPause));
    exports.Set("setBpm",
        Napi::Function::New(env, SetBpm));
    exports.Set("setTimeSignature",
        Napi::Function::New(env, SetTimeSignature));
    exports.Set("setLoopRegion",
        Napi::Function::New(env, SetLoopRegion));

    // Phase 2 — Config
    exports.Set("setBufferSize",
        Napi::Function::New(env, SetBufferSize));

    // Phase 2 — Metronome
    exports.Set("setMetronomeEnabled",
        Napi::Function::New(env, SetMetronomeEnabled));
    exports.Set("setMetronomeVolume",
        Napi::Function::New(env, SetMetronomeVolume));

    // Phase 2 — State queries
    exports.Set("getTransportState",
        Napi::Function::New(env, GetTransportState));
    exports.Set("getAudioConfig",
        Napi::Function::New(env, GetAudioConfig));

    return exports;
}

NODE_API_MODULE(calliope_addon, Init)
