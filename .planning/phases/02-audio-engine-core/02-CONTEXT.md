# Phase 2: Audio Engine Core - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — discuss skipped)

<domain>
## Phase Boundary

The C++ audio engine processes multi-track audio in real-time with stable playback, transport controls, and lock-free thread communication. This phase builds the audio processing core on top of Phase 1's JUCE/cmake-js bridge.

Delivers: real-time audio callback with multi-track mixing, transport state machine (play/stop/pause/loop with BPM/time signature), configurable buffer sizes (128-2048), master bus with insert chain, metronome click, and lock-free FIFO for audio-thread/UI-thread communication.

Does NOT deliver: instruments, effects, UI, or command dispatcher — those are Phases 3-10.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1 patterns:
- Async bridge pattern (ThreadSafeFunction + Promise) is established — extend for new engine APIs
- Atomic primitives for lock-free cross-thread state already proven in test_tone.cpp
- JUCE AudioDeviceManager lifecycle (mutex-guarded setup/teardown) is the existing pattern
- Engine namespace with static methods is the current C++ API surface

Requirements mapped to this phase: ENG-01, ENG-02, ENG-04, ENG-05, ENG-06, ARCH-04

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `calliope::Engine` static class (engine/include/calliope/engine.h) — extend with audio processing API
- `TestToneCallback` (engine/src/test_tone.cpp) — JUCE AudioIODeviceCallback pattern to evolve into real audio processor
- Native bridge pattern (native/src/bridge.cpp) — ThreadSafeFunction + Promise for all async calls
- IPC channel pattern (app/src/main/index.ts) — 'engine:*' namespace for new transport/config APIs

### Established Patterns
- std::atomic<T> for lock-free single-value cross-thread communication
- std::mutex for setup/teardown lifecycle only (not per-sample)
- std::unique_ptr for RAII resource management
- Promise-based async for all bridge calls
- contextBridge IPC with strict context isolation

### Integration Points
- engine/CMakeLists.txt — add new source files to calliope_engine static library
- native/src/bridge.cpp — add new bridge functions for transport, config, track management
- native/src/addon.cpp — register new exports
- app/src/main/index.ts — add IPC handlers for new engine APIs
- app/src/preload/index.ts — expose new APIs to renderer

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure phase. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — infrastructure phase.

</deferred>

---

*Phase: 02-audio-engine-core*
*Context gathered: 2026-03-28*
