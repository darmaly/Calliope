---
phase: 02-audio-engine-core
plan: 02
subsystem: audio-engine
tags: [juce, audio-processor-graph, metronome, master-bus, singleton, dsp]

# Dependency graph
requires:
  - phase: 02-audio-engine-core/01
    provides: "Transport (AudioPlayHead), LockFreeQueue, engine static class, CMake build"
provides:
  - "AudioGraph class managing AudioProcessorGraph with device I/O"
  - "MasterBusProcessor for master chain volume control"
  - "MetronomeProcessor generating beat-synced clicks from Transport PPQ"
  - "Engine singleton owning AudioGraph lifecycle"
  - "TransportStateInfo and AudioConfigInfo query structs"
affects: [02-audio-engine-core/03, 03-native-bridge, 05-effects-chain]

# Tech tracking
tech-stack:
  added: [juce_audio_utils]
  patterns: [AudioProcessorGraph node wiring, Meyer's singleton, GraphCallback transport advance]

key-files:
  created:
    - engine/include/calliope/audio_graph.h
    - engine/include/calliope/master_bus.h
    - engine/include/calliope/metronome.h
    - engine/src/audio_graph.cpp
    - engine/src/master_bus.cpp
    - engine/src/metronome.cpp
    - engine/tests/test_audio_graph.cpp
    - engine/tests/test_metronome.cpp
  modified:
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt

key-decisions:
  - "GraphCallback pattern: custom AudioIODeviceCallback advances Transport before delegating to AudioProcessorPlayer"
  - "Meyer's singleton for Engine instead of static class pattern from Phase 1"
  - "Raw pointers for graph-owned processors (graph owns unique_ptrs, Engine holds raw ptrs for access)"

patterns-established:
  - "AudioProcessor subclass pattern: BusesProperties constructor, stub overrides, atomic controls"
  - "AudioProcessorGraph wiring: addNode + addConnection with channel-pair connections"
  - "Integration test tagging: [integration] tag for tests requiring audio hardware"

requirements-completed: [ENG-01, ENG-02, ENG-04, ENG-06]

# Metrics
duration: 13min
completed: 2026-03-28
---

# Phase 02 Plan 02: Audio Graph & Engine Singleton Summary

**AudioProcessorGraph mixing architecture with metronome -> master -> output wiring, beat-synced click generation, and Engine singleton owning full audio lifecycle**

## Performance

- **Duration:** 13 min
- **Started:** 2026-03-28T05:54:02Z
- **Completed:** 2026-03-28T06:07:34Z
- **Tasks:** 2
- **Files modified:** 12

## Accomplishments
- AudioProcessorGraph wires metronome -> master bus -> audio output with stereo connections
- MetronomeProcessor generates decaying sine-wave clicks at beat boundaries using Transport PPQ position
- Engine refactored from static class to Meyer's singleton owning AudioGraph lifecycle
- 5 MetronomeProcessor unit tests + 7 AudioGraph integration tests, all passing (27 unit + 7 integration = 30 total assertions)

## Task Commits

Each task was committed atomically:

1. **Task 1: MasterBusProcessor, MetronomeProcessor, and AudioGraph classes** - `d73aee1` (feat)
2. **Task 2: Engine singleton refactor + AudioGraph/Metronome integration tests** - `a003bda` (feat)

## Files Created/Modified
- `engine/include/calliope/audio_graph.h` - AudioGraph class with AudioProcessorGraph, DeviceManager, Transport ownership
- `engine/include/calliope/master_bus.h` - MasterBusProcessor with atomic masterVolume
- `engine/include/calliope/metronome.h` - MetronomeProcessor with PPQ-synced click generation
- `engine/src/audio_graph.cpp` - Graph wiring, device init, GraphCallback for transport advance
- `engine/src/master_bus.cpp` - Pass-through processor applying master volume
- `engine/src/metronome.cpp` - Beat detection from PPQ, decaying sine burst generation
- `engine/include/calliope/engine.h` - Singleton with AudioGraph ownership, convenience wrappers
- `engine/src/engine.cpp` - Singleton impl, transport/metronome delegation, state queries
- `engine/tests/test_audio_graph.cpp` - 7 integration tests (init, 48kHz, buffer sizes, component access)
- `engine/tests/test_metronome.cpp` - 5 unit tests (defaults, disabled, no playhead, name, stereo)
- `engine/CMakeLists.txt` - Added 3 source files, juce_audio_utils dependency
- `engine/tests/CMakeLists.txt` - Added 2 test files

## Decisions Made
- GraphCallback pattern: advances Transport in audio callback before delegating to AudioProcessorPlayer, ensuring PPQ is current when processors read it
- Meyer's singleton for Engine: thread-safe initialization, owns AudioGraph via unique_ptr
- Raw pointers for graph-owned processors: AudioProcessorGraph owns the unique_ptrs, Engine stores raw ptrs for convenient access

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required

None - no external service configuration required.

## Known Stubs

None - all processors are functionally complete for their Phase 2 scope. MasterBusProcessor is intentionally a pass-through (insert chain deferred to Phase 5).

## Next Phase Readiness
- AudioGraph ready for track node insertion (Plan 03 track management)
- Engine singleton API ready for native bridge exposure (Phase 3)
- MasterBusProcessor ready for effect chain insertion (Phase 5)

---
*Phase: 02-audio-engine-core*
*Completed: 2026-03-28*
