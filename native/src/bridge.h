#pragma once
#include <napi.h>

// See bridge.cpp for the threading model and the argument shapes.

// Engine information and test tone
Napi::Value GetEngineInfo(const Napi::CallbackInfo& info);
Napi::Value StartTestTone(const Napi::CallbackInfo& info);
Napi::Value StopTestTone(const Napi::CallbackInfo& info);

// Lifecycle
Napi::Value InitialiseEngine(const Napi::CallbackInfo& info);
Napi::Value ShutdownEngine(const Napi::CallbackInfo& info);
Napi::Value SetBufferSize(const Napi::CallbackInfo& info);

// Transport and metronome
Napi::Value TransportPlay(const Napi::CallbackInfo& info);
Napi::Value TransportStop(const Napi::CallbackInfo& info);
Napi::Value TransportPause(const Napi::CallbackInfo& info);
Napi::Value SetBpm(const Napi::CallbackInfo& info);
Napi::Value SetTimeSignature(const Napi::CallbackInfo& info);
Napi::Value SetLoopRegion(const Napi::CallbackInfo& info);
Napi::Value SetMetronomeEnabled(const Napi::CallbackInfo& info);
Napi::Value SetMetronomeVolume(const Napi::CallbackInfo& info);

// State queries
Napi::Value GetTransportState(const Napi::CallbackInfo& info);
Napi::Value GetAudioConfig(const Napi::CallbackInfo& info);
Napi::Value GetMeterLevels(const Napi::CallbackInfo& info);

// Tracks
Napi::Value AddTrack(const Napi::CallbackInfo& info);
Napi::Value RemoveTrack(const Napi::CallbackInfo& info);
Napi::Value ResetProject(const Napi::CallbackInfo& info);

// Clips
Napi::Value AddClip(const Napi::CallbackInfo& info);
Napi::Value UpdateClip(const Napi::CallbackInfo& info);
Napi::Value RemoveClip(const Napi::CallbackInfo& info);
Napi::Value ClearClips(const Napi::CallbackInfo& info);

// Commands, parameters and project state
Napi::Value DispatchCommand(const Napi::CallbackInfo& info);
Napi::Value CommandUndo(const Napi::CallbackInfo& info);
Napi::Value CommandRedo(const Napi::CallbackInfo& info);
Napi::Value GetProjectState(const Napi::CallbackInfo& info);
Napi::Value LoadProjectState(const Napi::CallbackInfo& info);
Napi::Value GetParameterIds(const Napi::CallbackInfo& info);
Napi::Value DescribeParameters(const Napi::CallbackInfo& info);
Napi::Value SubscribeToEvents(const Napi::CallbackInfo& info);
Napi::Value UnsubscribeFromEvents(const Napi::CallbackInfo& info);

// Export
Napi::Value GetExportFormats(const Napi::CallbackInfo& info);
Napi::Value ExportAudio(const Napi::CallbackInfo& info);
Napi::Value ExportStems(const Napi::CallbackInfo& info);
