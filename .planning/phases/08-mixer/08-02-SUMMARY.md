---
phase: 08-mixer
plan: 02
subsystem: audio-engine
tags: [metering, rms, peak, atomics, napi, ipc, real-time]

requires:
  - phase: 05-effects
    provides: InsertChainProcessor and MasterBusProcessor with processBlock pipeline
  - phase: 03-command
    provides: Command dispatcher and parameter registry for volume/pan commands
provides:
  - Per-track and master RMS/peak level metering via atomic reads in processBlock
  - getMeterLevels bridge endpoint returning all meter data in one IPC call
  - setTrackVolume and setTrackPan preload convenience methods
affects: [08-mixer plan 03 (mixer UI), 10-integration]

tech-stack:
  added: []
  patterns: [MeterData atomic struct pattern for real-time meter computation, single-call meter aggregation via AllMeterLevels]

key-files:
  created: []
  modified:
    - engine/include/calliope/insert_chain_processor.h
    - engine/src/insert_chain_processor.cpp
    - engine/include/calliope/master_bus.h
    - engine/src/master_bus.cpp
    - engine/include/calliope/audio_graph.h
    - engine/src/audio_graph.cpp
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - native/src/bridge.cpp
    - native/src/bridge.h
    - native/src/addon.cpp
    - app/src/main/index.ts
    - app/src/main/native-bridge.ts
    - app/src/preload/index.ts
    - app/src/renderer/types/calliope.d.ts

key-decisions:
  - "MeterData struct with std::atomic<float> and memory_order_relaxed for lock-free audio thread metering"
  - "Single getMeterLevels() call aggregates all track + master meters to minimize IPC round-trips for 30Hz polling"

patterns-established:
  - "MeterData atomic struct: per-processor struct with rmsLeft/rmsRight/peakLeft/peakRight atomics, computed at end of processBlock"
  - "AllMeterLevels aggregation: AudioGraph reads all processor meter data in one method for efficient bridge consumption"

requirements-completed: [MIX-04, MIX-01]

duration: 37min
completed: 2026-03-29
---

# Phase 8 Plan 2: Metering Infrastructure Summary

**Real-time RMS/peak level metering from C++ audio thread through atomic reads, native bridge, IPC, to renderer preload API**

## Performance

- **Duration:** 37 min
- **Started:** 2026-03-29T19:10:07Z
- **Completed:** 2026-03-29T19:47:16Z
- **Tasks:** 2
- **Files modified:** 15

## Accomplishments
- Per-track and master bus RMS/peak level computation in audio thread processBlock using lock-free atomics
- Single getMeterLevels() bridge endpoint returning all track + master meters in one IPC round-trip
- setTrackVolume and setTrackPan preload convenience methods routing through existing parameter.set command

## Task Commits

Each task was committed atomically:

1. **Task 1: C++ metering infrastructure** - `9684765` (feat)
2. **Task 2: Bridge endpoint, IPC handler, preload API** - `3d662d9` (feat)

## Files Created/Modified
- `engine/include/calliope/insert_chain_processor.h` - Added MeterData struct with atomic RMS/peak fields and getMeterData() accessor
- `engine/src/insert_chain_processor.cpp` - RMS/peak computation at end of processBlock using memory_order_relaxed
- `engine/include/calliope/master_bus.h` - Added MeterData struct and getMeterData() accessor
- `engine/src/master_bus.cpp` - RMS/peak computation after master volume application
- `engine/include/calliope/audio_graph.h` - AllMeterLevels struct and getMeterLevels() method
- `engine/src/audio_graph.cpp` - getMeterLevels() reads all track + master atomics
- `engine/include/calliope/engine.h` - getMeterLevels() delegation method
- `engine/src/engine.cpp` - getMeterLevels() implementation delegating to AudioGraph
- `native/src/bridge.cpp` - GetMeterLevels bridge function with ThreadSafeFunction pattern
- `native/src/bridge.h` - GetMeterLevels declaration
- `native/src/addon.cpp` - getMeterLevels export registration
- `app/src/main/index.ts` - engine:meter:getLevels IPC handler
- `app/src/main/native-bridge.ts` - getMeterLevels type in NativeAddon interface
- `app/src/preload/index.ts` - getMeterLevels, setTrackVolume, setTrackPan preload methods
- `app/src/renderer/types/calliope.d.ts` - getMeterLevels, setTrackVolume, setTrackPan type declarations

## Decisions Made
- MeterData uses std::atomic<float> with memory_order_relaxed -- consistent with Phase 2 decision for all Transport atomics (sufficient for single-value audio thread access)
- Single getMeterLevels() aggregation call rather than per-track calls -- minimizes IPC overhead for 30Hz polling from the mixer UI
- setTrackVolume/setTrackPan route through existing parameter.set command path -- no new C++ command classes needed

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Worktree deep path nesting caused JUCE juceaide build failure (path too long for intermediate build artifacts). Verified build by temporarily applying changes to main repo. This is an environmental limitation of the worktree location, not a code issue.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Meter data pipeline complete end-to-end: C++ atomics -> bridge -> IPC -> preload API
- Plan 03 (mixer UI) can now call window.calliope.getMeterLevels() at 30Hz for real-time level meters
- Volume/pan controls ready for mixer channel strip faders and knobs

## Self-Check: PASSED

All 15 files found. Both commits (9684765, 3d662d9) verified. All acceptance criteria content checks passed.

---
*Phase: 08-mixer*
*Completed: 2026-03-29*
