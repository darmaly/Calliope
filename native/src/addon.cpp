#include <napi.h>
#include "bridge.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("getEngineInfo",
        Napi::Function::New(env, GetEngineInfo));
    exports.Set("startTestTone",
        Napi::Function::New(env, StartTestTone));
    exports.Set("stopTestTone",
        Napi::Function::New(env, StopTestTone));
    return exports;
}

NODE_API_MODULE(calliope_addon, Init)
