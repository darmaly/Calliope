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

    // Phase 3 — Command dispatch
    exports.Set("dispatchCommand",
        Napi::Function::New(env, DispatchCommand));
    exports.Set("commandUndo",
        Napi::Function::New(env, CommandUndo));
    exports.Set("commandRedo",
        Napi::Function::New(env, CommandRedo));
    exports.Set("getProjectState",
        Napi::Function::New(env, GetProjectState));
    exports.Set("getParameterIds",
        Napi::Function::New(env, GetParameterIds));
    exports.Set("subscribeToEvents",
        Napi::Function::New(env, SubscribeToEvents));
    exports.Set("unsubscribeFromEvents",
        Napi::Function::New(env, UnsubscribeFromEvents));

    // Phase 8 — Metering
    exports.Set("getMeterLevels",
        Napi::Function::New(env, GetMeterLevels));

    // Phase 9 — Project save/load
    exports.Set("saveProject",
        Napi::Function::New(env, SaveProject));
    exports.Set("loadProject",
        Napi::Function::New(env, LoadProject));

    // Phase 9 — Export
    exports.Set("exportAudio",
        Napi::Function::New(env, ExportAudio));
    exports.Set("exportStems",
        Napi::Function::New(env, ExportStems));
    exports.Set("loadProjectState",
        Napi::Function::New(env, LoadProjectState));

    return exports;
}

NODE_API_MODULE(calliope_addon, Init)
