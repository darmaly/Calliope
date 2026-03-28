---
phase: 06-timeline-arrangement
plan: 02
subsystem: ui
tags: [react, pixi.js, pixi-viewport, zustand, tailwind, lucide-react, timeline, daw]

requires:
  - phase: 06-01
    provides: "Timeline types, Zustand store, beat-math utils, TRACK_COLORS, pixi-setup"
provides:
  - "TimelineView layout with 200px track header column and canvas area"
  - "PixiJS canvas with pixi-viewport scroll/zoom"
  - "Grid layer rendering bar/beat lines scaled to zoom and grid resolution"
  - "CSS playhead overlay synced to engine transport via rAF"
  - "Track headers with color strip, mute/solo/arm buttons"
  - "Toolbar with grid resolution dropdown, snap toggle, zoom controls"
  - "AddTrackButton for creating new tracks"
affects: [06-03, 06-04, 07-piano-roll, 08-mixer]

tech-stack:
  added: [lucide-react, pixi-viewport]
  patterns: ["@pixi/react Application + extend() for custom PixiJS components", "useShallow from zustand/shallow for multi-value selectors", "rAF polling at 30Hz for engine transport sync", "CSS playhead overlay positioned via beatToPixel", "ResizeObserver for dynamic canvas sizing", "Canvas 2D for ruler rendering with DPR scaling"]

key-files:
  created:
    - app/src/renderer/components/timeline/TimelineView.tsx
    - app/src/renderer/components/timeline/TimelineCanvas.tsx
    - app/src/renderer/components/timeline/TimelineToolbar.tsx
    - app/src/renderer/components/timeline/TimelineRuler.tsx
    - app/src/renderer/components/timeline/GridLayer.tsx
    - app/src/renderer/components/timeline/Playhead.tsx
    - app/src/renderer/components/tracks/TrackHeaderList.tsx
    - app/src/renderer/components/tracks/TrackHeader.tsx
    - app/src/renderer/components/tracks/AddTrackButton.tsx
    - app/src/renderer/components/shared/GridResolutionSelect.tsx
    - app/src/renderer/hooks/use-playhead.ts
  modified:
    - app/src/renderer/App.tsx
    - app/src/renderer/components/timeline/pixi-setup.ts
    - app/package.json

key-decisions:
  - "Canvas 2D ruler instead of PixiJS (simpler, isolated from viewport scroll)"
  - "Manual wheel event handler instead of pixi-viewport plugins for scroll/zoom (better Ctrl/Shift modifier control)"
  - "CSS playhead overlay instead of PixiJS child (simpler z-ordering, no viewport transform issues)"

patterns-established:
  - "TimelineView as top-level layout composing toolbar, ruler, headers, canvas, and playhead"
  - "useShallow() for all multi-value Zustand selectors to prevent unnecessary re-renders"
  - "usePlayhead() rAF hook with 30Hz throttle for engine transport sync"
  - "extend({ Viewport }) with declare module for pixi-viewport @pixi/react integration"

requirements-completed: [TL-01, TL-04, TL-05]

duration: 5min
completed: 2026-03-28
---

# Phase 6 Plan 2: Timeline Visual Layout Summary

**Multi-track timeline layout with PixiJS canvas, pixi-viewport scroll/zoom, grid overlay, CSS playhead, and track headers with mute/solo/arm controls**

## Performance

- **Duration:** 5 min (322s)
- **Started:** 2026-03-28T15:20:33Z
- **Completed:** 2026-03-28T15:25:55Z
- **Tasks:** 2
- **Files modified:** 14

## Accomplishments
- Full timeline layout: 200px track header column on left, toolbar on top, ruler below toolbar, PixiJS canvas filling remaining space
- PixiJS canvas with pixi-viewport providing scroll (wheel/shift-wheel) and zoom (ctrl/cmd-wheel) with clamped pixelsPerBeat (6-192)
- Grid layer rendering bar-boundary major lines at 0.12 alpha and subdivision minor lines at 0.05 alpha, both scaling with zoom level
- CSS playhead overlay at accent color #6c63ff, positioned from engine transport ppqPosition via rAF polling at ~30Hz
- Track headers with 4px color strip from TRACK_COLORS palette, mute (amber), solo (green), arm (red) toggle buttons
- Toolbar with GridResolutionSelect (1/4, 1/8, 1/16, 1/32), Snap toggle with accent highlight, ZoomIn/ZoomOut buttons

## Task Commits

Each task was committed atomically:

1. **Task 1: Create TimelineView layout, TrackHeaders, Toolbar, GridResolutionSelect, and replace App.tsx** - `daf82a6` (feat)
2. **Task 2: Create PixiJS TimelineCanvas with Viewport, GridLayer, and Playhead with rAF engine sync** - `25780c4` (feat)

## Files Created/Modified
- `app/src/renderer/App.tsx` - Replaced placeholder with TimelineView layout, minimal engine status indicator
- `app/src/renderer/components/timeline/TimelineView.tsx` - Top-level layout composing all timeline sub-components
- `app/src/renderer/components/timeline/TimelineCanvas.tsx` - PixiJS Application with pixi-viewport and GridLayer
- `app/src/renderer/components/timeline/TimelineToolbar.tsx` - Grid resolution, snap toggle, zoom controls
- `app/src/renderer/components/timeline/TimelineRuler.tsx` - Canvas 2D ruler with bar/beat numbers and tick marks
- `app/src/renderer/components/timeline/GridLayer.tsx` - PixiJS Graphics drawing vertical/horizontal grid lines
- `app/src/renderer/components/timeline/Playhead.tsx` - CSS positioned div at accent color synced to transport
- `app/src/renderer/components/tracks/TrackHeaderList.tsx` - Scrollable track header column with scrollY sync
- `app/src/renderer/components/tracks/TrackHeader.tsx` - Individual track header with color strip, M/S/Arm buttons
- `app/src/renderer/components/tracks/AddTrackButton.tsx` - Plus button calling addTrack() action
- `app/src/renderer/components/shared/GridResolutionSelect.tsx` - Grid resolution dropdown (1/4 through 1/32)
- `app/src/renderer/hooks/use-playhead.ts` - rAF hook polling engine transport at ~30Hz
- `app/src/renderer/components/timeline/pixi-setup.ts` - Fixed: removed non-existent MeshMaterial export
- `app/package.json` - Added lucide-react and pixi-viewport dependencies

## Decisions Made
- Used Canvas 2D for the ruler instead of PixiJS -- simpler, isolated from viewport scrolling, DPR-aware
- Handled wheel events manually on the container element instead of using pixi-viewport's built-in drag/wheel plugins -- gives precise control over Ctrl+wheel zoom vs Shift+wheel horizontal scroll vs plain wheel vertical scroll
- CSS playhead overlay positioned via beatToPixel() instead of PixiJS child -- avoids viewport transform complications and simplifies z-ordering

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed pixi-setup.ts MeshMaterial export**
- **Found during:** Task 2 (build verification)
- **Issue:** pixi-setup.ts imported `MeshMaterial` from pixi.js v8, but this export does not exist in PixiJS 8.17.1
- **Fix:** Removed MeshMaterial from the import and extend() call
- **Files modified:** app/src/renderer/components/timeline/pixi-setup.ts
- **Verification:** electron-vite build succeeds
- **Committed in:** 25780c4 (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Pre-existing bug in pixi-setup.ts from Phase 6 Plan 1. Fix was necessary for build to pass. No scope creep.

## Issues Encountered
None beyond the pixi-setup.ts fix noted above.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Timeline layout complete with all visual components rendering
- Canvas ready for clip components (MidiClip, AudioClip) to be added in Plan 03
- Track headers wired to store actions, ready for clip drag/drop in Plan 04
- Playhead sync active, will animate during playback once engine is running

---
*Phase: 06-timeline-arrangement*
*Completed: 2026-03-28*
