---
phase: 02-audio-engine-core
plan: 03
subsystem: native-bridge
tags: [node-addon-api, ipc, electron, preload, contextbridge, napi, threadsafefunction]

# Dependency graph
requires:
  - phase: 02-audio-engine-core/02
    provides: "Engine singleton with AudioGraph, Transport, Metronome, state query structs"
  - phase: 01-build-system-app-shell/02
    provides: "Electron app shell, IPC bridge pattern, native addon loading, preload contextBridge"
provides:
  - "13 native bridge functions exposing all Phase 2 engine APIs to JavaScript"
  - "IPC handlers for engine lifecycle, transport, metronome, and config"
  - "contextBridge preload exposing window.calliope API with full Phase 2 surface"
  - "TypeScript type declarations for TransportState and AudioConfig"
affects: [03-command-dispatcher, 06-timeline-arrangement, 08-mixer, 10-application-integration]

# Tech tracking
tech-stack:
  added: []
  patterns: [ThreadSafeFunction + Promise bridge for all engine APIs, namespaced IPC channels (engine:transport:*, engine:metronome:*, engine:config:*)]

key-files:
  modified:
    - native/src/bridge.h
    - native/src/bridge.cpp
    - native/src/addon.cpp
    - app/src/main/native-bridge.ts
    - app/src/main/index.ts
    - app/src/preload/index.ts
    - app/src/renderer/types/calliope.d.ts

key-decisions:
  - "ThreadSafeFunction + thread pattern used for ALL bridge functions (even fast ones) for consistency and safety"
  - "Namespaced IPC channels: engine:transport:*, engine:metronome:*, engine:config:* for clear organization"

patterns-established:
  - "Bridge function pattern: extract args -> spawn thread -> call Engine singleton -> resolve Promise via TSFN"
  - "IPC channel naming: engine:{subsystem}:{action} for all engine-related channels"
  - "NativeAddon interface extends with typed Promise returns matching C++ struct shapes"

requirements-completed: [ENG-01, ENG-02, ENG-04, ENG-05, ENG-06, ARCH-04]

# Metrics
duration: 3min
completed: 2026-03-28
---

# Phase 02 Plan 03: Native Bridge & IPC Wiring Summary

**13 ThreadSafeFunction bridge functions wiring Engine lifecycle, transport, metronome, and config APIs from C++ through IPC to window.calliope, verified with audible metronome playback**

## Performance

- **Duration:** 3 min (post-checkpoint summary only; Task 1 execution was in prior session)
- **Started:** 2026-03-28T06:25:02Z
- **Completed:** 2026-03-28T06:28:00Z
- **Tasks:** 2 (1 auto + 1 human-verify checkpoint)
- **Files modified:** 7

## Accomplishments
- All Phase 2 C++ engine APIs (initialise, shutdown, transport play/stop/pause, BPM, time signature, loop region, buffer size, metronome enable/volume, state queries) callable from JavaScript
- IPC handlers route renderer requests through main process to native addon with typed parameters
- Human verified: metronome clicks audible, synced to BPM, transport controls work, state queries return correct values

## Task Commits

Each task was committed atomically:

1. **Task 1: Native bridge functions + addon registration + IPC + preload + types** - `bb1f879` (feat)
2. **Task 2: Human verification of metronome playback** - checkpoint approved, no code changes

## Files Created/Modified
- `native/src/bridge.h` - 13 new bridge function declarations
- `native/src/bridge.cpp` - 13 bridge function implementations using ThreadSafeFunction + Promise pattern
- `native/src/addon.cpp` - 13 new exports.Set registrations for all bridge functions
- `app/src/main/native-bridge.ts` - Extended NativeAddon interface + TransportState/AudioConfig types
- `app/src/main/index.ts` - 13 ipcMain.handle registrations for engine:* channels
- `app/src/preload/index.ts` - Extended contextBridge with all Phase 2 APIs
- `app/src/renderer/types/calliope.d.ts` - TransportState, AudioConfig interfaces, extended CalliopeAPI

## Decisions Made
- Used ThreadSafeFunction + thread pattern for ALL bridge functions (even atomic reads) for consistency and to avoid blocking the Node.js event loop if Engine internals ever change
- Namespaced IPC channels (engine:transport:play, engine:metronome:setEnabled, etc.) for clear organization and future middleware/logging

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required

None - no external service configuration required.

## Known Stubs

None - all bridge functions are fully wired to the C++ engine APIs.

## Next Phase Readiness
- Phase 2 complete: all audio engine core APIs accessible from JavaScript
- Command dispatcher (Phase 3) can route commands through window.calliope
- UI phases can call transport/metronome/config directly via the typed API
- Engine initialise/shutdown lifecycle available for app startup/teardown

## Self-Check: PASSED

- Commit bb1f879: FOUND
- 02-03-SUMMARY.md: FOUND

---
*Phase: 02-audio-engine-core*
*Completed: 2026-03-28*
