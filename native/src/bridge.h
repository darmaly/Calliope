#pragma once
#include <napi.h>

// Phase 1
Napi::Value GetEngineInfo(const Napi::CallbackInfo& info);
Napi::Value StartTestTone(const Napi::CallbackInfo& info);
Napi::Value StopTestTone(const Napi::CallbackInfo& info);

// Phase 2 — Engine lifecycle
Napi::Value InitialiseEngine(const Napi::CallbackInfo& info);
Napi::Value ShutdownEngine(const Napi::CallbackInfo& info);

// Phase 2 — Transport
Napi::Value TransportPlay(const Napi::CallbackInfo& info);
Napi::Value TransportStop(const Napi::CallbackInfo& info);
Napi::Value TransportPause(const Napi::CallbackInfo& info);
Napi::Value SetBpm(const Napi::CallbackInfo& info);
Napi::Value SetTimeSignature(const Napi::CallbackInfo& info);
Napi::Value SetLoopRegion(const Napi::CallbackInfo& info);

// Phase 2 — Config
Napi::Value SetBufferSize(const Napi::CallbackInfo& info);

// Phase 2 — Metronome
Napi::Value SetMetronomeEnabled(const Napi::CallbackInfo& info);
Napi::Value SetMetronomeVolume(const Napi::CallbackInfo& info);

// Phase 2 — State queries
Napi::Value GetTransportState(const Napi::CallbackInfo& info);
Napi::Value GetAudioConfig(const Napi::CallbackInfo& info);

// Phase 3 — Command dispatch (Phase 4 instrument commands flow through DispatchCommand)
Napi::Value DispatchCommand(const Napi::CallbackInfo& info);
Napi::Value CommandUndo(const Napi::CallbackInfo& info);
Napi::Value CommandRedo(const Napi::CallbackInfo& info);
Napi::Value GetProjectState(const Napi::CallbackInfo& info);
Napi::Value GetParameterIds(const Napi::CallbackInfo& info);
Napi::Value SubscribeToEvents(const Napi::CallbackInfo& info);
Napi::Value UnsubscribeFromEvents(const Napi::CallbackInfo& info);

// Phase 8 — Metering
Napi::Value GetMeterLevels(const Napi::CallbackInfo& info);

// Phase 9 — Project save/load
Napi::Value SaveProject(const Napi::CallbackInfo& info);
Napi::Value LoadProject(const Napi::CallbackInfo& info);

// Phase 9 — Export
Napi::Value ExportAudio(const Napi::CallbackInfo& info);
Napi::Value ExportStems(const Napi::CallbackInfo& info);
Napi::Value LoadProjectState(const Napi::CallbackInfo& info);

// Phase 10.1 — Clip scheduling
Napi::Value AddClip(const Napi::CallbackInfo& info);
Napi::Value RemoveClip(const Napi::CallbackInfo& info);
Napi::Value UpdateClip(const Napi::CallbackInfo& info);
Napi::Value ClearClips(const Napi::CallbackInfo& info);
