---
phase: 01-build-system-app-shell
plan: 01
subsystem: build-system
tags: [cmake, juce, native-addon, node-addon-api, cmake-js]
dependency_graph:
  requires: []
  provides: [calliope_addon.node, calliope_engine, cmake-build-system]
  affects: [01-02]
tech_stack:
  added: [JUCE 8.0.12, cmake-js 8.0.0, node-addon-api 8.7.0, vitest]
  patterns: [ThreadSafeFunction async bridge, single CMake tree, JUCE static lib]
key_files:
  created:
    - CMakeLists.txt
    - engine/CMakeLists.txt
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - engine/src/test_tone.cpp
    - native/CMakeLists.txt
    - native/src/addon.cpp
    - native/src/bridge.cpp
    - native/src/bridge.h
    - package.json
    - .gitmodules
    - .gitignore
    - tsconfig.json
    - vitest.config.ts
    - test/native.test.ts
    - test/app.test.ts
  modified: []
decisions:
  - id: D-PNPM-BUILDS
    summary: "Added pnpm.onlyBuiltDependencies for @swc/core, electron, esbuild to approve postinstall scripts"
  - id: D-ADDON-LOADS-NODE
    summary: "Native addon successfully loads in plain Node.js despite being built against Electron headers -- ABI compatible on this platform"
metrics:
  duration: 7m 27s
  completed: "2026-03-28T04:43:54Z"
  tasks_completed: 2
  tasks_total: 2
---

# Phase 01 Plan 01: CMake Build System & Native Addon Bridge Summary

C++ audio engine compiled as static library linking 6 JUCE modules, exposed to Node.js via cmake-js native addon using ThreadSafeFunction async pattern for all bridge methods.

## What Was Built

### Task 1: Monorepo Scaffold
- Root `CMakeLists.txt` orchestrating JUCE submodule, engine static lib, and native addon targets
- `engine/CMakeLists.txt` linking juce_core, juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_events, juce_dsp
- `native/CMakeLists.txt` with node-addon-api/cmake-js integration (NAPI_VERSION=9)
- `package.json` with arch-agnostic cmake-js config (runtime: electron, runtimeVersion: 35.7.5)
- JUCE 8.0.12 as git submodule at vendor/JUCE
- Vitest configured with test stubs for native addon and Electron app

### Task 2: C++ Engine & Native Bridge
- `Engine` class with static methods: getJuceVersion, getAudioDevices, startTestTone, stopTestTone
- Test tone implementation using juce::AudioIODeviceCallback with sine wave generation (thread-safe with mutex)
- Native bridge using Napi::ThreadSafeFunction for all three methods (async from day one per D-08)
- NODE_API_MODULE registration exposing getEngineInfo, startTestTone, stopTestTone

### Verification Results
- cmake-js build completes without errors
- `calliope_addon.node` binary produced in build/Release/ (3.9MB)
- Addon loads in Node.js and returns: `{"juceVersion":"JUCE v8.0.12","audioDevices":["CoreAudio: MacBook Pro Speakers"]}`
- Vitest runs with 5 todo stubs (all pending, as expected)

## Commits

| Task | Commit | Message |
|------|--------|---------|
| 1 | 821056b | chore(01-01): scaffold monorepo with build system and JUCE submodule |
| 2 | 32dbeff | feat(01-01): implement C++ engine library and native addon bridge |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] pnpm build script approval required**
- **Found during:** Task 1
- **Issue:** pnpm 10.x requires explicit approval of postinstall scripts (security feature). @swc/core, electron, and esbuild builds were blocked.
- **Fix:** Added `pnpm.onlyBuiltDependencies` to package.json listing approved packages.
- **Files modified:** package.json
- **Commit:** 821056b

## Known Stubs

None -- all code is functional. Test stubs in test/native.test.ts and test/app.test.ts are intentionally `.todo()` placeholders to be implemented in Plan 02 when Electron shell is available.
