---
phase: 06-timeline-arrangement
plan: 01
subsystem: timeline-data-layer
tags: [pixi, tailwind, zustand, types, utilities, tdd]
dependency_graph:
  requires: []
  provides: [timeline-types, timeline-store, beat-math, track-colors, pixi-setup]
  affects: [06-02, 06-03, 06-04]
tech_stack:
  added: [pixi.js@8.17.1, "@pixi/react@8.0.5", pixi-viewport@6.0.3, lucide-react@1.7.0, tailwindcss@4.2.2, "@tailwindcss/vite@4.2.2"]
  patterns: [zustand-store, tdd-red-green, tailwind-v4-vite-plugin]
key_files:
  created:
    - app/src/renderer/types/timeline.ts
    - app/src/renderer/stores/timeline-store.ts
    - app/src/renderer/utils/beat-math.ts
    - app/src/renderer/utils/colors.ts
    - app/src/renderer/components/timeline/pixi-setup.ts
    - test/beat-math.test.ts
    - test/timeline-store.test.ts
  modified:
    - package.json
    - pnpm-lock.yaml
    - app/electron.vite.config.ts
    - app/src/renderer/App.css
    - app/src/renderer/types/calliope.d.ts
decisions:
  - Tailwind CSS v4 via @tailwindcss/vite plugin (no config file, @import entry point)
  - Zustand v5 create<> pattern with Set for selectedClipIds
  - crypto.randomUUID() for track and clip ID generation
  - Color index cycles modulo 8 for auto-assigned track colors
metrics:
  duration: 189s
  completed: "2026-03-28T15:18:41Z"
  tasks_completed: 2
  tasks_total: 2
  tests_added: 38
  files_changed: 12
---

# Phase 06 Plan 01: Timeline Data Layer & Dependencies Summary

Zustand timeline store with typed Track/Clip/TimelineState, beat-math utilities, PixiJS registration, and Tailwind CSS v4 configured for electron-vite renderer.

## Task Results

### Task 1: Install dependencies, configure Tailwind, create types and utilities with tests

- **Commit:** 422c315
- **TDD:** RED (module not found) -> GREEN (10/10 pass)
- Installed 6 packages: pixi.js, @pixi/react, pixi-viewport, lucide-react, tailwindcss, @tailwindcss/vite
- Configured Tailwind CSS v4 in electron-vite renderer via @tailwindcss/vite plugin
- Replaced App.css with `@import "tailwindcss"` entry point
- Created timeline type system: Track, Clip, TimelineState, TimelineActions, GridResolution, LoopRegion
- Implemented beat-math utilities: beatToPixel, pixelToBeat, snapToBeat
- Defined TRACK_COLORS palette (8 colors)
- Registered PixiJS components (Container, Graphics, Mesh, Text, MeshGeometry, MeshMaterial)
- Extended calliope.d.ts with Phase 3-5 API methods

### Task 2: Create Zustand timeline store with all actions and tests

- **Commit:** b764814
- **TDD:** RED (module not found) -> GREEN (28/28 pass)
- Created useTimelineStore with complete action set: track CRUD, clip CRUD, selection, viewport, grid, loop, transport mirror
- Track auto-naming with cycling color index
- removeTrack cascades to delete associated clips
- duplicateClip offsets new clip by original's lengthBeats
- selectClip supports single and multi-select modes
- Initial state: pixelsPerBeat=24, gridResolution=0.25, snapEnabled=true, bpm=120

## Verification

```
pnpm vitest run test/beat-math.test.ts test/timeline-store.test.ts
  2 passed (2)
  38 passed (38)
```

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None - all data layer functionality is fully implemented and tested.

## Self-Check: PASSED

All 7 created files verified on disk. Both commit hashes (422c315, b764814) found in git log.
