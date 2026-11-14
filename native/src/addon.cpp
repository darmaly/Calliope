#include <napi.h>
#include "bridge.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Engine information and test tone
    exports.Set("getEngineInfo", Napi::Function::New(env, GetEngineInfo));
    exports.Set("startTestTone", Napi::Function::New(env, StartTestTone));
    exports.Set("stopTestTone", Napi::Function::New(env, StopTestTone));

    // Lifecycle
    exports.Set("initialiseEngine", Napi::Function::New(env, InitialiseEngine));
    exports.Set("shutdownEngine", Napi::Function::New(env, ShutdownEngine));
    exports.Set("setBufferSize", Napi::Function::New(env, SetBufferSize));

    // Transport and metronome
    exports.Set("transportPlay", Napi::Function::New(env, TransportPlay));
    exports.Set("transportStop", Napi::Function::New(env, TransportStop));
    exports.Set("transportPause", Napi::Function::New(env, TransportPause));
    exports.Set("setBpm", Napi::Function::New(env, SetBpm));
    exports.Set("setTimeSignature", Napi::Function::New(env, SetTimeSignature));
    exports.Set("setLoopRegion", Napi::Function::New(env, SetLoopRegion));
    exports.Set("setMetronomeEnabled", Napi::Function::New(env, SetMetronomeEnabled));
    exports.Set("setMetronomeVolume", Napi::Function::New(env, SetMetronomeVolume));

    // State queries
    exports.Set("getTransportState", Napi::Function::New(env, GetTransportState));
    exports.Set("getAudioConfig", Napi::Function::New(env, GetAudioConfig));
    exports.Set("getMeterLevels", Napi::Function::New(env, GetMeterLevels));

    // Tracks
    exports.Set("addTrack", Napi::Function::New(env, AddTrack));
    exports.Set("removeTrack", Napi::Function::New(env, RemoveTrack));
    exports.Set("resetProject", Napi::Function::New(env, ResetProject));

    // Clips
    exports.Set("addClip", Napi::Function::New(env, AddClip));
    exports.Set("updateClip", Napi::Function::New(env, UpdateClip));
    exports.Set("removeClip", Napi::Function::New(env, RemoveClip));
    exports.Set("clearClips", Napi::Function::New(env, ClearClips));

    // Commands, parameters and project state
    exports.Set("dispatchCommand", Napi::Function::New(env, DispatchCommand));
    exports.Set("commandUndo", Napi::Function::New(env, CommandUndo));
    exports.Set("commandRedo", Napi::Function::New(env, CommandRedo));
    exports.Set("getProjectState", Napi::Function::New(env, GetProjectState));
    exports.Set("loadProjectState", Napi::Function::New(env, LoadProjectState));
    exports.Set("getParameterIds", Napi::Function::New(env, GetParameterIds));
    exports.Set("describeParameters", Napi::Function::New(env, DescribeParameters));
    exports.Set("subscribeToEvents", Napi::Function::New(env, SubscribeToEvents));
    exports.Set("unsubscribeFromEvents", Napi::Function::New(env, UnsubscribeFromEvents));

    // Export
    exports.Set("getExportFormats", Napi::Function::New(env, GetExportFormats));
    exports.Set("exportAudio", Napi::Function::New(env, ExportAudio));
    exports.Set("exportStems", Napi::Function::New(env, ExportStems));

    return exports;
}

NODE_API_MODULE(calliope_addon, Init)
