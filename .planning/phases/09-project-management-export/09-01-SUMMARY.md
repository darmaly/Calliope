---
phase: 09-project-management-export
plan: 01
subsystem: project-management
tags: [json-serialization, electron-dialog, autosave, zustand, ipc]

# Dependency graph
requires:
  - phase: 05-effects-processing
    provides: Full engine state with instruments, effects, transport for serialization
  - phase: 03-command-dispatcher-state
    provides: ProjectState toJson/fromJson, ParameterRegistry for state restoration
provides:
  - ProjectSerializer C++ class with versioned JSON file format
  - Save/load bridge functions through native addon
  - Electron file dialog integration for save/saveAs/load
  - Autosave timer in main process with configurable interval
  - Zustand project store with dirty tracking
  - Full preload API for project operations
affects: [10-application-integration, 09-02, 09-03]

# Tech tracking
tech-stack:
  added: []
  patterns: [versioned-json-envelope, main-process-autosave-timer, dirty-tracking-via-command-events]

key-files:
  created:
    - engine/include/calliope/project_serializer.h
    - engine/src/project_serializer.cpp
    - app/src/renderer/stores/project-store.ts
  modified:
    - native/src/bridge.h
    - native/src/bridge.cpp
    - native/src/addon.cpp
    - app/src/main/index.ts
    - app/src/main/native-bridge.ts
    - app/src/preload/index.ts
    - app/src/renderer/types/calliope.d.ts
    - engine/CMakeLists.txt

key-decisions:
  - "Versioned JSON envelope format {version, appVersion, savedAt, data} for forward compatibility"
  - ".ltproj file extension for LuneyTunes project files"
  - "Autosave in main process (not renderer) for reliability independent of UI state"
  - "ParameterRegistry setter pattern for state restoration instead of direct atomic writes"

patterns-established:
  - "Versioned envelope: all project files use {version: N, data: {...}} for migration support"
  - "Main-process autosave: timer + dirty flag + filePath check before save"
  - "Project store dirty tracking: command events auto-mark dirty, saves mark clean"

requirements-completed: [PROJ-01, PROJ-02]

# Metrics
duration: 5min
completed: 2026-03-29
---

# Phase 9 Plan 1: Project Save/Load & Autosave Summary

**Versioned JSON project serialization with Electron file dialogs, main-process autosave timer, and Zustand dirty tracking**

## Performance

- **Duration:** 5 min
- **Started:** 2026-03-29T22:24:33Z
- **Completed:** 2026-03-29T22:29:45Z
- **Tasks:** 2
- **Files modified:** 12

## Accomplishments
- Full project state serializes to versioned JSON file (.ltproj) with format version, app version, and timestamp
- Engine state restoration on load via ParameterRegistry setter functions for all instrument/effect parameters
- Electron native file dialogs for save/saveAs/load with proper file extension filtering
- Configurable autosave timer in main process (default 120s) that auto-saves when dirty and has a file path
- Zustand project store with dirty tracking, autosave notifications, and save/load actions

## Task Commits

Each task was committed atomically:

1. **Task 1: C++ ProjectSerializer and bridge wiring for save/load** - `cb1f1c4` (feat)
2. **Task 2: Autosave timer, project store, and dirty tracking** - `589c9b1` (feat)

## Files Created/Modified
- `engine/include/calliope/project_serializer.h` - Static saveToFile/loadFromFile with versioned JSON envelope
- `engine/src/project_serializer.cpp` - File I/O with version envelope wrapping ProjectState JSON
- `engine/CMakeLists.txt` - Added project_serializer.cpp to build
- `native/src/bridge.h` - SaveProject/LoadProject bridge declarations
- `native/src/bridge.cpp` - Bridge functions with TSFN pattern, state restoration via registry setters
- `native/src/addon.cpp` - Registered saveProject/loadProject exports
- `app/src/main/index.ts` - IPC handlers for project:save/saveAs/load/new + autosave timer + dialog integration
- `app/src/main/native-bridge.ts` - NativeAddon interface extended with saveProject/loadProject
- `app/src/preload/index.ts` - Preload API: projectSave, projectSaveAs, projectLoad, projectNew, autosave config, events
- `app/src/renderer/types/calliope.d.ts` - CalliopeAPI type updated with all project methods
- `app/src/renderer/stores/project-store.ts` - Zustand store for project metadata, dirty tracking, save/load actions

## Decisions Made
- Versioned JSON envelope format {version, appVersion, savedAt, data} for forward-compatible file format migration
- .ltproj file extension for LuneyTunes project files
- Autosave runs in main process rather than renderer for reliability (renderer crashes should not break autosave)
- State restoration uses ParameterRegistry getter/setter pattern (not direct atomic writes) for consistency with existing parameter system

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed ParameterRegistry API usage in LoadProject**
- **Found during:** Task 1 (bridge wiring)
- **Issue:** Plan assumed registry.setParameter() method, but ParameterRegistry uses getParameter()->setter() pattern
- **Fix:** Changed to lambda helper using registry.getParameter(id)->setter(juce::var(value))
- **Files modified:** native/src/bridge.cpp
- **Verification:** Code uses correct API matching existing parameter_commands.cpp pattern
- **Committed in:** cb1f1c4 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Essential correction for API compatibility. No scope creep.

## Issues Encountered
None

## Known Stubs
None - all data paths are wired end-to-end. Timeline/piano-roll clip data is not yet included in ProjectState serialization (this is pre-existing scope, not a stub introduced by this plan). The existing ProjectState already captures engine state (transport, instruments, effects); UI-only state like timeline clips lives in Zustand stores and will need separate persistence in a future plan.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Project save/load foundation complete, ready for Phase 09 Plan 02 (audio export: WAV/MP3/FLAC)
- Timeline/clip state persistence can be added as a future enhancement (UI store serialization)
- Phase 10 can use project save/load for keyboard shortcuts (Cmd+S, Cmd+O)

---
*Phase: 09-project-management-export*
*Completed: 2026-03-29*
