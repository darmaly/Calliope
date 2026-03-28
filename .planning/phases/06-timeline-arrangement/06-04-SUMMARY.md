---
phase: 06-timeline-arrangement
plan: 04
subsystem: ui
tags: [react, pixijs, zustand, electron, timeline, context-menu, keyboard-shortcuts, ipc]

requires:
  - phase: 06-02
    provides: Timeline layout shell with PixiJS canvas, track headers, toolbar, ruler
  - phase: 06-03
    provides: Clip rendering components (MIDI/Audio), selection box, loop region, clip operations
provides:
  - Fully interactive timeline with clip create/select/move/resize via mouse
  - Context menus for track management and clip operations
  - Global keyboard shortcuts for delete, undo/redo, duplicate, play/pause, snap, zoom
  - Engine event sync hook for real-time state updates
  - Loop region creation on ruler via Shift+drag
affects: [piano-roll, mixer, application-integration]

tech-stack:
  added: []
  patterns: [portal-based-context-menu, global-keyboard-shortcut-hook, engine-event-sync-hook, pointer-event-interaction-handlers]

key-files:
  created:
    - app/src/renderer/components/shared/ContextMenu.tsx
    - app/src/renderer/hooks/use-context-menu.ts
    - app/src/renderer/hooks/use-engine-sync.ts
    - app/src/renderer/hooks/use-keyboard-shortcuts.ts
  modified:
    - app/src/renderer/components/timeline/TimelineCanvas.tsx
    - app/src/renderer/components/timeline/TimelineView.tsx
    - app/src/renderer/components/timeline/TimelineRuler.tsx
    - app/src/renderer/components/timeline/ClipContainer.tsx
    - app/src/renderer/components/timeline/GridLayer.tsx
    - app/src/renderer/components/tracks/TrackHeader.tsx
    - engine/src/engine.cpp

key-decisions:
  - "Portal-based context menu with document click listener for outside-click dismissal"
  - "Pointer events for all interaction handlers (not mouse events) for unified input handling"
  - "CSS playhead overlay maintained from 06-02 (simpler z-ordering than PixiJS child)"
  - "Engine null guard added to getTransportState to prevent SIGSEGV on uninitialized engine"

patterns-established:
  - "ContextMenu pattern: portal-based, z-50, dark theme, destructive item styling"
  - "useContextMenu hook pattern: show(event, items) / close() for menu state management"
  - "useEngineSync pattern: onCommandEvent subscription with graceful calliope absence"
  - "useKeyboardShortcuts pattern: document keydown with input/textarea exclusion"

requirements-completed: [TL-01, TL-02, TL-03, TL-04, TL-05, TL-06]

duration: ~45min
completed: 2026-03-28
---

# Phase 06 Plan 04: Timeline Interaction Handlers Summary

**Fully interactive timeline arrangement view with clip CRUD via mouse, context menus for track/clip management, keyboard shortcuts, and engine event sync**

## Performance

- **Duration:** ~45 min (across checkpoint verification iterations)
- **Started:** 2026-03-28T15:30:00Z (approx)
- **Completed:** 2026-03-28T16:58:19Z
- **Tasks:** 2/2 (1 auto + 1 human-verify checkpoint)
- **Files modified:** 11

## Accomplishments
- Clip creation via click+drag on empty track area with grid snapping
- Clip selection (click, Shift+click, Ctrl+click), move, and resize with edge detection
- Context menus for tracks (add/delete/rename/color/duplicate) and clips (delete/duplicate/split)
- Global keyboard shortcuts: Delete, Ctrl+Z undo, Ctrl+Shift+Z redo, Ctrl+D duplicate, Space play/pause, S snap toggle, Ctrl+/-  zoom
- Loop region creation on ruler via Shift+click+drag
- Engine event sync hook for real-time transport state updates from C++ engine
- Human-verified: dark theme layout, track management, clip interactions, scroll sync, zoom all working

## Task Commits

Each task was committed atomically:

1. **Task 1: Wire clip components, interaction handlers, context menus, keyboard shortcuts** - `6d8cd38` (feat)
2. **Task 1 fix: PixiJS rendering, scroll sync, clip creation, layout fixes** - `52ed2aa` (fix)

**Task 2:** Human-verify checkpoint -- approved by user after iterative verification.

## Files Created/Modified
- `app/src/renderer/components/shared/ContextMenu.tsx` - Reusable portal-based right-click context menu
- `app/src/renderer/hooks/use-context-menu.ts` - Context menu state management hook
- `app/src/renderer/hooks/use-engine-sync.ts` - Engine command event subscription hook
- `app/src/renderer/hooks/use-keyboard-shortcuts.ts` - Global keyboard shortcut handler
- `app/src/renderer/components/timeline/TimelineCanvas.tsx` - Added clip containers, interaction handlers for create/select/move/resize
- `app/src/renderer/components/timeline/TimelineView.tsx` - Wired useEngineSync, useKeyboardShortcuts hooks
- `app/src/renderer/components/timeline/TimelineRuler.tsx` - Added loop region creation via Shift+drag
- `app/src/renderer/components/timeline/ClipContainer.tsx` - Fixed rendering and infinite loop issues
- `app/src/renderer/components/timeline/GridLayer.tsx` - Fixed grid rendering
- `app/src/renderer/components/tracks/TrackHeader.tsx` - Added right-click context menu with track management options
- `engine/src/engine.cpp` - Added null guard for getTransportState to prevent SIGSEGV

## Decisions Made
- Portal-based context menu with document click listener for outside-click dismissal
- Pointer events (not mouse events) for all interaction handlers -- unified input handling
- Engine null guard added to getTransportState to prevent SIGSEGV on uninitialized engine
- Manual wheel event handler maintained for scroll/zoom with Ctrl/Shift modifier control

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Engine SIGSEGV on getTransportState**
- **Found during:** Task 1 verification (app launch crash)
- **Issue:** Engine getTransportState accessed transport pointer before initialization, causing segfault
- **Fix:** Added null guard in engine.cpp getTransportState
- **Files modified:** engine/src/engine.cpp
- **Verification:** App launches without crash
- **Committed in:** 52ed2aa

**2. [Rule 1 - Bug] PixiJS rendering issues (viewport, renderer guard, ClipContainer infinite loop)**
- **Found during:** Task 1 verification (visual verification)
- **Issue:** Multiple rendering bugs: viewport reference errors, missing renderer guard, ClipContainer causing infinite re-render loop
- **Fix:** Fixed pixiContainer references, added renderer existence check, broke infinite loop in ClipContainer
- **Files modified:** TimelineCanvas.tsx, ClipContainer.tsx, GridLayer.tsx
- **Verification:** App renders correctly with grid lines and track dividers
- **Committed in:** 52ed2aa

**3. [Rule 1 - Bug] Scroll sync and layout issues**
- **Found during:** Task 2 human verification
- **Issue:** Vertical scroll not synced between track sidebar and canvas, horizontal scroll not working, track dividers not visible enough
- **Fix:** Fixed scroll sync handlers, horizontal scroll behavior, increased track divider prominence
- **Files modified:** TimelineView.tsx, TimelineCanvas.tsx
- **Verification:** Human verified scroll sync, horizontal scroll, and visual layout
- **Committed in:** 52ed2aa

---

**Total deviations:** 3 auto-fixed (3 bugs)
**Impact on plan:** All fixes necessary for correct rendering and interaction. No scope creep.

## Issues Encountered
- PixiJS viewport/container reference model required iterative debugging to get correct parent-child relationships
- ClipContainer had an infinite re-render loop due to dependency array issues in useEffect
- Engine null pointer crash required C++ side fix (not just JS side)

## Checkpoint: Human Verification

**Status:** APPROVED

The user verified all interactive behaviors:
- Dark theme timeline layout with track headers, PixiJS canvas, grid lines, track dividers
- Track creation via + button, auto-incrementing names, color strips
- Clip creation via click+drag with grid snapping
- Clip resize by dragging edges
- Horizontal and vertical scroll (synced between sidebar and canvas)
- Zoom controls
- Context menus for tracks and clips
- Toolbar controls

## User Setup Required

None - no external service configuration required.

## Known Stubs

None - all interaction handlers are wired to real Zustand store actions. Engine commands (C++ side) are wrapped in try/catch as noted in 06-03 decisions, since C++ clip commands are not yet implemented (store-only for now).

## Next Phase Readiness
- Timeline arrangement view is fully interactive -- ready for Phase 7 (Piano Roll) which will open from clip double-click
- Phase 8 (Mixer) can proceed independently -- track state is available via Zustand store
- Phase 10 (Application Integration) will wire timeline into unified layout

## Self-Check: PASSED

All created files verified present. All commit hashes verified in git log.

---
*Phase: 06-timeline-arrangement*
*Completed: 2026-03-28*
