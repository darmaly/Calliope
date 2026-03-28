---
phase: 04-instruments
plan: 03
subsystem: audio-engine
tags: [juce, audio-graph, midi, instruments, commands, ipc, parameter-registry]

# Dependency graph
requires:
  - phase: 04-instruments/04-01
    provides: "PolySynthProcessor and BassSynthProcessor classes"
  - phase: 04-instruments/04-02
    provides: "DrumMachineProcessor with sample loading"
provides:
  - "Instruments wired as AudioProcessorGraph nodes connected to master bus"
  - "36 instrument parameters registered in ParameterRegistry"
  - "NoteOnCommand, NoteOffCommand, LoadSampleCommand for instrument control"
  - "Bridge dispatch for instrument.noteOn, instrument.noteOff, drumMachine.loadSample"
  - "Preload convenience API: instrumentNoteOn, instrumentNoteOff, drumMachineLoadSample"
  - "ProjectState JSON includes full instrument state"
affects: [05-effects, 06-ui, 07-ai-integration]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "AudioProcessor polymorphism for instrument command dispatch via dynamic_cast"
    - "Namespaced parameter IDs: polysynth.*, basssynth.*, drumMachine.*"
    - "Non-undoable commands for real-time MIDI events"
    - "Convenience preload methods wrapping dispatchCommand for ergonomic renderer API"

key-files:
  created:
    - engine/include/calliope/commands/instrument_commands.h
    - engine/src/commands/instrument_commands.cpp
    - engine/tests/test_instrument_commands.cpp
  modified:
    - engine/include/calliope/audio_graph.h
    - engine/src/audio_graph.cpp
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - engine/include/calliope/project_state.h
    - engine/src/project_state.cpp
    - native/src/bridge.h
    - native/src/bridge.cpp
    - app/src/main/index.ts
    - app/src/main/native-bridge.ts
    - app/src/preload/index.ts
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt

key-decisions:
  - "AudioProcessor& polymorphism with dynamic_cast for instrument resolution in commands"
  - "All instrument parameters use lambda getter/setter pattern with std::memory_order_relaxed"
  - "DrumMachine pad names serialized as JSON array in ProjectState"

patterns-established:
  - "Instrument command pattern: NoteOn/NoteOff take AudioProcessor& and resolve type via dynamic_cast"
  - "Parameter namespace convention: instrumentname.paramname (polysynth.filterCutoff)"
  - "Preload convenience methods wrap dispatchCommand for common operations"

requirements-completed: [INST-04]

# Metrics
duration: 7min
completed: 2026-03-28
---

# Phase 04 Plan 03: Instrument Wiring Summary

**All three instruments wired into AudioGraph as graph nodes with 36 registered parameters, MIDI command dispatch, and full bridge/IPC integration**

## Performance

- **Duration:** 7 min
- **Started:** 2026-03-28T09:12:11Z
- **Completed:** 2026-03-28T09:19:28Z
- **Tasks:** 2
- **Files modified:** 16

## Accomplishments
- Wired PolySynth, BassSynth, and DrumMachine as AudioProcessorGraph nodes connected stereo to master bus
- Registered all 36 instrument parameters in ParameterRegistry with namespaced IDs (19 polysynth, 15 basssynth, 1 drumMachine)
- Created NoteOnCommand, NoteOffCommand, and LoadSampleCommand with 16 Catch2 tests
- Extended bridge DispatchCommand with instrument commands and added preload convenience API
- ProjectState JSON now includes full instrument state with all parameters and drum machine pad names

## Task Commits

Each task was committed atomically:

1. **Task 1: Wire instruments into AudioGraph, register parameters, create instrument commands** - `b53d45b` (feat)
2. **Task 2: Extend bridge with instrument commands, add IPC and preload API** - `84ac16b` (feat)

## Files Created/Modified
- `engine/include/calliope/commands/instrument_commands.h` - NoteOnCommand, NoteOffCommand, LoadSampleCommand class declarations
- `engine/src/commands/instrument_commands.cpp` - Command implementations using dynamic_cast for instrument resolution
- `engine/tests/test_instrument_commands.cpp` - 16 Catch2 tests for commands, properties, and parameter registration
- `engine/include/calliope/audio_graph.h` - Added instrument pointers, node ptrs, and accessor methods
- `engine/src/audio_graph.cpp` - Create instrument graph nodes, connect to master, accessors, shutdown cleanup
- `engine/include/calliope/engine.h` - Convenience instrument access methods
- `engine/src/engine.cpp` - Register 36 instrument parameters, instrument access delegation
- `engine/include/calliope/project_state.h` - PolySynthData, BassSynthData, DrumMachineData structs
- `engine/src/project_state.cpp` - Instrument state serialization/deserialization and engine snapshot
- `native/src/bridge.cpp` - instrument.noteOn, instrument.noteOff, drumMachine.loadSample dispatch
- `app/src/preload/index.ts` - instrumentNoteOn, instrumentNoteOff, drumMachineLoadSample convenience methods

## Decisions Made
- Used AudioProcessor& polymorphism with dynamic_cast to resolve instrument types in commands rather than storing separate typed references -- simpler API, one constructor signature
- All 36 instrument parameters use the existing lambda getter/setter pattern with std::memory_order_relaxed, consistent with Phase 3 patterns
- DrumMachine pad names serialized as a JSON array in ProjectState for full sample state capture

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Known Stubs
None - all data sources are wired; no placeholder or mock data.

## Next Phase Readiness
- All Phase 4 instruments complete: PolySynth, BassSynth, DrumMachine created, tested, wired, and controllable
- Ready for Phase 5 (Effects) which will add effect processors to the audio graph using the same pattern
- All instrument parameters accessible from renderer via dispatchCommand or convenience API
- Full test suite at 118 tests, all passing

---
*Phase: 04-instruments*
*Completed: 2026-03-28*
