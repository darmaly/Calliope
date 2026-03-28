---
phase: 03-command-dispatcher-state
plan: 02
subsystem: engine
tags: [command-pattern, undo-redo, transport, parameter-registry, JUCE, C++]

# Dependency graph
requires:
  - phase: 03-command-dispatcher-state/01
    provides: Command base class, CommandDispatcher, ParameterRegistry, ProjectState, EventTypes
  - phase: 02-audio-engine-core
    provides: Transport, MetronomeProcessor, MasterBusProcessor, AudioGraph, Engine singleton
provides:
  - Transport command classes (Play, Stop, Pause, SetBpm, SetTimeSignature, SetLoopRegion)
  - Metronome command classes (SetMetronomeEnabled, SetMetronomeVolume)
  - Master bus command class (SetMasterVolume)
  - Generic SetParameterCommand for registry-based parameter access
  - Engine owns CommandDispatcher and ParameterRegistry with 6 registered parameters
  - getCommandDispatcher(), getParameterRegistry(), getProjectState() on Engine
affects: [03-command-dispatcher-state/03, bridge-layer, ui-layer]

# Tech tracking
tech-stack:
  added: []
  patterns: [dependency-injection for command testability, non-undoable command pattern]

key-files:
  created:
    - engine/include/calliope/commands/transport_commands.h
    - engine/include/calliope/commands/parameter_commands.h
    - engine/src/commands/transport_commands.cpp
    - engine/src/commands/parameter_commands.cpp
    - engine/tests/test_transport_commands.cpp
    - engine/tests/test_parameter_commands.cpp
  modified:
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt

key-decisions:
  - "Commands accept component references (Transport&, MetronomeProcessor&) via constructor for testability without audio hardware"
  - "6 parameters registered: transport.bpm, transport.timeSigNumerator, transport.timeSigDenominator, metronome.enabled, metronome.volume, master.volume"

patterns-established:
  - "Dependency injection: Commands take references to components they operate on, not Engine::getInstance()"
  - "Non-undoable pattern: Play/Stop/Pause override isUndoable() to false, dispatcher executes them directly"

requirements-completed: [ARCH-01, ARCH-05, PROJ-03]

# Metrics
duration: 3min
completed: 2026-03-28
---

# Phase 03 Plan 02: Concrete Command Classes and Engine Integration Summary

**Transport, metronome, and master bus command classes with undo support, plus Engine integration with CommandDispatcher and ParameterRegistry owning 6 registered parameters**

## Performance

- **Duration:** 3 min (192s)
- **Started:** 2026-03-28T06:50:23Z
- **Completed:** 2026-03-28T06:53:35Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- 9 concrete Command subclasses covering all existing DAW operations (play, stop, pause, setBpm, setTimeSignature, setLoopRegion, setMetronomeEnabled, setMetronomeVolume, setMasterVolume)
- Generic SetParameterCommand addresses any registered parameter by string ID through the ParameterRegistry
- Engine singleton owns CommandDispatcher and ParameterRegistry with 6 parameters registered at init
- 25 new Catch2 tests (48 assertions) for command classes; 80 total tests passing with no regression

## Task Commits

Each task was committed atomically:

1. **Task 1: Create transport and parameter command classes** - `0cb7078` (feat)
2. **Task 2: Wire Engine to own CommandDispatcher and ParameterRegistry** - `9a2d82f` (feat)

## Files Created/Modified
- `engine/include/calliope/commands/transport_commands.h` - Play/Stop/Pause/SetBpm/SetTimeSignature/SetLoopRegion/SetMetronomeEnabled/SetMetronomeVolume/SetMasterVolume command classes
- `engine/include/calliope/commands/parameter_commands.h` - Generic SetParameterCommand class
- `engine/src/commands/transport_commands.cpp` - Transport and metronome/master command implementations
- `engine/src/commands/parameter_commands.cpp` - SetParameterCommand perform/undo via registry lookup
- `engine/tests/test_transport_commands.cpp` - 20 Catch2 tests for transport/metronome/master commands
- `engine/tests/test_parameter_commands.cpp` - 5 Catch2 tests for SetParameterCommand
- `engine/include/calliope/engine.h` - Added dispatcher_, paramRegistry_, getCommandDispatcher(), getParameterRegistry(), getProjectState()
- `engine/src/engine.cpp` - registerParameters() with 6 parameter definitions, dispatcher/registry accessors
- `engine/CMakeLists.txt` - Added command source files
- `engine/tests/CMakeLists.txt` - Added command test files

## Decisions Made
- Commands accept component references via constructor (Transport&, MetronomeProcessor&, MasterBusProcessor&) rather than calling Engine::getInstance() directly -- enables unit testing without audio hardware initialization
- Existing Engine convenience methods (transportPlay, setBpm, etc.) preserved for backward compatibility; bridge layer will route through dispatcher in Plan 03

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- All concrete commands ready for dispatcher-based routing in Plan 03 (bridge integration)
- Engine exposes getCommandDispatcher() and getParameterRegistry() for bridge layer to use
- Existing Phase 2 bridge functions continue working (backward compatible)

---
*Phase: 03-command-dispatcher-state*
*Completed: 2026-03-28*
