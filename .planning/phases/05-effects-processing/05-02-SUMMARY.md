---
phase: 05-effects-processing
plan: 02
subsystem: audio-engine
tags: [juce, audio-graph, insert-chain, effect-commands, parameter-registry, project-state, undo-redo]

requires:
  - phase: 05-01
    provides: "InsertChain, ParametricEQ, Compressor, Reverb, Delay, Limiter effect processors"
provides:
  - "InsertChainProcessor wrapping InsertChain as AudioProcessor graph node"
  - "Per-track and master insert chain routing in AudioGraph"
  - "4 undoable effect commands (insert, remove, reorder, bypass)"
  - "Dynamic effect parameter registration/unregistration via ParameterRegistry"
  - "ProjectState serialization of effect chains with parameters"
affects: [05-effects-processing, 06-bridge, 09-project-management]

tech-stack:
  added: []
  patterns: [insert-chain-processor-wrapper, dynamic-parameter-registration, effect-command-factory]

key-files:
  created:
    - engine/include/calliope/insert_chain_processor.h
    - engine/src/insert_chain_processor.cpp
    - engine/include/calliope/commands/effect_commands.h
    - engine/src/commands/effect_commands.cpp
    - engine/tests/test_effect_commands.cpp
  modified:
    - engine/include/calliope/audio_graph.h
    - engine/src/audio_graph.cpp
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - engine/include/calliope/parameter_registry.h
    - engine/src/parameter_registry.cpp
    - engine/include/calliope/project_state.h
    - engine/src/project_state.cpp
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt

key-decisions:
  - "Consistent InsertChainProcessor pattern for master bus (separate graph node between masterBus and output rather than embedding chain in MasterBusProcessor)"
  - "Effect parameter ID scheme: effects.{trackId}.{slotIndex}.{paramName} for dynamic registration"
  - "createEffect factory function maps short type names (eq, compressor, reverb, delay, limiter) to processor classes"
  - "ReorderEffectCommand unregisters and re-registers ALL effect parameters for the track (slot indices shift)"

patterns-established:
  - "InsertChainProcessor wrapper: AudioProcessor subclass wrapping InsertChain for graph routing"
  - "Dynamic parameter registration: registerEffectParameters/unregisterEffectParameters pattern with type-based dispatch"
  - "Effect command pattern: commands take InsertChain& and Engine& references for chain manipulation and parameter registration"

requirements-completed: [FX-06, ENG-03]

duration: 8min
completed: 2026-03-28
---

# Phase 5 Plan 2: Effect Commands and Graph Integration Summary

**Per-track and master insert chain routing in AudioGraph with 4 undoable effect commands, dynamic parameter registration, and ProjectState serialization**

## Performance

- **Duration:** 8 min (463s)
- **Started:** 2026-03-28T09:49:06Z
- **Completed:** 2026-03-28T09:57:00Z
- **Tasks:** 2
- **Files modified:** 15

## Accomplishments
- InsertChainProcessor wraps InsertChain as AudioProcessor, routing instruments through per-track chains before master bus and master through master chain before output
- Four undoable effect commands (insert, remove, reorder, bypass) with createEffect factory for all 5 effect types
- Dynamic ParameterRegistry with removeParameter and removeParametersWithPrefix for effect lifecycle management
- Engine registers/unregisters effect parameters automatically using effects.{trackId}.{slotIndex}.{paramName} scheme
- ProjectState captures and serializes complete effect chain state including per-effect parameters
- 24 new Catch2 tests, all 174 tests pass

## Task Commits

Each task was committed atomically:

1. **Task 1: InsertChainProcessor, AudioGraph routing, and effect parameter registration** - `67bd194` (feat)
2. **Task 2: Effect commands, ProjectState serialization, and Catch2 tests** - `7938351` (feat)

## Files Created/Modified
- `engine/include/calliope/insert_chain_processor.h` - InsertChainProcessor AudioProcessor wrapping InsertChain
- `engine/src/insert_chain_processor.cpp` - Implementation with delegation to InsertChain
- `engine/include/calliope/commands/effect_commands.h` - 4 effect commands + createEffect factory
- `engine/src/commands/effect_commands.cpp` - Command implementations with undo/redo
- `engine/tests/test_effect_commands.cpp` - 24 Catch2 tests for all commands, registry, and serialization
- `engine/include/calliope/audio_graph.h` - Added insert chain processor pointers and accessors
- `engine/src/audio_graph.cpp` - Rewired routing: instrument -> chain -> master -> masterChain -> output
- `engine/include/calliope/engine.h` - Added getInsertChain, registerEffectParameters, unregisterEffectParameters
- `engine/src/engine.cpp` - Dynamic parameter registration for all 5 effect types
- `engine/include/calliope/parameter_registry.h` - Added removeParameter and removeParametersWithPrefix
- `engine/src/parameter_registry.cpp` - Implementation of removal methods
- `engine/include/calliope/project_state.h` - Added EffectSlotData, InsertChainData structs
- `engine/src/project_state.cpp` - Serialization, deserialization, and snapshotFromEngine for effect chains
- `engine/CMakeLists.txt` - Added insert_chain_processor.cpp and effect_commands.cpp
- `engine/tests/CMakeLists.txt` - Added test_effect_commands.cpp

## Decisions Made
- Used consistent InsertChainProcessor pattern for master bus (separate graph node) rather than embedding in MasterBusProcessor -- keeps all 4 chains symmetric
- Effect parameter ID scheme uses dot-notation `effects.{trackId}.{slotIndex}.{paramName}` for hierarchical organization
- createEffect factory maps short type names to processor classes for clean command interface
- ReorderEffectCommand does full re-registration of all track parameters since slot indices shift during reorder

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Known Stubs
None - all effect chain functionality is fully wired.

## Next Phase Readiness
- Effect commands and graph routing are fully operational from C++
- Ready for plan 03 (bridge layer exposure) to make effects accessible from JS/Electron
- ProjectState serialization captures effect state for future project save/load (Phase 9)

## Self-Check: PASSED

All created files exist. Both commit hashes verified.

---
*Phase: 05-effects-processing*
*Completed: 2026-03-28*
