#pragma once
#include <napi.h>

Napi::Value GetEngineInfo(const Napi::CallbackInfo& info);
Napi::Value StartTestTone(const Napi::CallbackInfo& info);
Napi::Value StopTestTone(const Napi::CallbackInfo& info);
