---
phase: 06-timeline-arrangement
verified: 2026-03-28T17:03:35Z
status: human_needed
score: 5/5 must-haves verified
human_verification:
  - test: "Launch app with pnpm dev from Calliope root, verify dark-themed timeline layout renders"
    expected: "App shows timeline with toolbar, track headers column (200px), ruler, and PixiJS canvas area — not the old placeholder"
    why_human: "Visual rendering requires a running Electron process with WebGL; cannot verify programmatically"
  - test: "Click Add Track — verify track header appears with auto-incremented name and color strip"
    expected: "Track 1 appears with a 4px colored left border; adding more tracks shows Track 2, Track 3 with cycling colors"
    why_human: "DOM interaction in Electron renderer; requires live app"
  - test: "Click+drag on empty track area — verify MIDI clip is created with grid snapping"
    expected: "A colored rounded rectangle appears on the canvas at the dragged beat range; releases snap to grid"
    why_human: "PixiJS canvas rendering + pointer interaction requires live app"
  - test: "Click a clip, then Shift+click another — verify multi-selection with accent border (#6c63ff)"
    expected: "Selected clips show a 2px purple border; multiple clips can be selected together"
    why_human: "Visual selection state requires live app"
  - test: "Drag a selected clip to a new position — verify move with grid snapping"
    expected: "Clip moves to the dragged beat position, snapping to the current grid resolution"
    why_human: "Pointer capture + PixiJS canvas requires live app"
  - test: "Hover near clip edge — verify cursor changes to col-resize"
    expected: "Cursor switches to resize icon within 6px of either clip edge; dragging resizes the clip"
    why_human: "CSS cursor change on hover requires live app"
  - test: "Right-click a track header — verify context menu with Add/Delete/Rename/Color/Duplicate options"
    expected: "Dark-themed portal menu appears at mouse position with correct items; destructive Delete is red"
    why_human: "Context menu portal rendering requires live app"
  - test: "Right-click a clip — verify context menu with Delete/Duplicate/Split at Playhead"
    expected: "Clip context menu appears; Delete Clip removes it; Duplicate Clip creates a copy"
    why_human: "Requires live app"
  - test: "Press Delete with clips selected — verify clips are removed"
    expected: "All selected clips disappear from the canvas"
    why_human: "Keyboard shortcut + store mutation requires live app"
  - test: "Press Ctrl+Z — verify undo"
    expected: "Last action is undone (engine undo IPC fires if engine is running)"
    why_human: "Engine IPC requires running native process"
  - test: "Shift+click+drag on ruler — verify loop region tinted overlay"
    expected: "A semi-transparent purple overlay spans the dragged beat range on the canvas"
    why_human: "Canvas overlay rendering requires live app"
  - test: "Scroll vertically — verify track headers stay in sync with canvas"
    expected: "Track headers translateY matches canvas scrollY; both scroll in lockstep"
    why_human: "DOM + PixiJS sync requires live app"
  - test: "Ctrl+scroll to zoom — verify grid lines scale with pixelsPerBeat"
    expected: "Grid lines spread apart (zoom in) or compress (zoom out); ruler bar numbers adjust"
    why_human: "WebGL canvas rendering requires live app"
---

# Phase 6: Timeline & Arrangement Verification Report

**Phase Goal:** Users can arrange music on a multi-track horizontal timeline with clips, tracks, and grid snapping
**Verified:** 2026-03-28T17:03:35Z
**Status:** human_needed (all automated checks passed; visual/interactive behaviors require live Electron app)
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths (from Success Criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|---------|
| 1 | Multi-track timeline displays with horizontal scrolling, zoom, and a moving playhead | VERIFIED | TimelineCanvas.tsx: wheel event handler for scroll/zoom; Playhead.tsx: CSS div positioned by `beatToPixel(currentBeat, pixelsPerBeat, scrollX)`; use-playhead.ts: rAF polling getTransportState at ~30Hz |
| 2 | MIDI clips can be created, moved, resized, copied, and deleted on tracks | VERIFIED | TimelineCanvas.tsx: full pointer event handler (create on drag, move with originals tracking, resize-left/right with edge detection); clip-operations.ts: all 5 CRUD functions; keyboard shortcuts: Delete, Ctrl+D for duplicate |
| 3 | Audio clips display waveforms and can be placed on tracks | VERIFIED | AudioClip.tsx: renders waveform from `clip.peakData` via `peaksToVertices()`; falls back to dashed center line when no peakData; ClipContainer routes clip.type to correct component |
| 4 | Tracks can be added, removed, reordered, renamed, and color-coded | VERIFIED | timeline-store.ts: addTrack, removeTrack, renameTrack, reorderTrack, setTrackColor all implemented; TrackHeader.tsx: inline rename on double-click + context menu; AddTrackButton.tsx: calls addTrack() |
| 5 | Snap-to-grid works with configurable resolution (1/4, 1/8, 1/16, 1/32 notes) | VERIFIED | GridResolutionSelect.tsx: dropdown with 4 options mapping to GridResolution values; TimelineCanvas.tsx: all drag operations apply `snapToBeat(beat, gridResolution)` when `snapEnabled`; toolbar snap toggle; 53 unit tests passing |

**Score:** 5/5 truths verified

### Required Artifacts

| Artifact | Expected | Level 1 (Exists) | Level 2 (Substantive) | Level 3 (Wired) | Status |
|----------|----------|------------------|-----------------------|-----------------|--------|
| `app/src/renderer/types/timeline.ts` | Track, Clip, TimelineState interfaces | EXISTS | 91 lines, full type system | Imported by store, components | VERIFIED |
| `app/src/renderer/stores/timeline-store.ts` | Zustand store with all actions | EXISTS | 192 lines, 20+ actions | Used by all timeline components | VERIFIED |
| `app/src/renderer/utils/beat-math.ts` | Beat/pixel conversion + snap | EXISTS | 3 exported functions | Used by Canvas, Ruler, Playhead | VERIFIED |
| `app/src/renderer/utils/colors.ts` | 8-color track palette | EXISTS | 8 entries, '#4a9eff' first | Used by MidiClip, AudioClip, TrackHeader | VERIFIED |
| `app/src/renderer/components/timeline/pixi-setup.ts` | PixiJS component registration | EXISTS | extend() with 5 components | Imported by TimelineCanvas | VERIFIED |
| `app/src/renderer/components/timeline/TimelineView.tsx` | Top-level layout | EXISTS | 89 lines, full layout | Rendered by App.tsx | VERIFIED |
| `app/src/renderer/components/timeline/TimelineCanvas.tsx` | PixiJS canvas with interactions | EXISTS | 466 lines, full interaction system | Rendered by TimelineView | VERIFIED |
| `app/src/renderer/components/timeline/GridLayer.tsx` | Grid lines PixiJS component | EXISTS | 74 lines, major+minor+track dividers | Rendered in CanvasContent | VERIFIED |
| `app/src/renderer/components/timeline/Playhead.tsx` | CSS playhead div | EXISTS | 28 lines, beatToPixel positioned | Rendered in TimelineView | VERIFIED |
| `app/src/renderer/components/timeline/TimelineRuler.tsx` | Ruler with loop region creation | EXISTS | 197 lines, Shift+drag loop region | Rendered in TimelineView | VERIFIED |
| `app/src/renderer/components/timeline/TimelineToolbar.tsx` | Toolbar with snap/zoom/grid | EXISTS | 68 lines, GridResolutionSelect+snap+zoom | Rendered in TimelineView | VERIFIED |
| `app/src/renderer/components/timeline/MidiClip.tsx` | PixiJS MIDI clip | EXISTS | 79 lines, rounded rect + top bar + label | Used by ClipContainer | VERIFIED |
| `app/src/renderer/components/timeline/AudioClip.tsx` | PixiJS audio clip with waveform | EXISTS | 114 lines, peaksToVertices waveform | Used by ClipContainer | VERIFIED |
| `app/src/renderer/components/timeline/ClipContainer.tsx` | Per-track clip container | EXISTS | 61 lines, filters clips by trackId | Used by TimelineCanvas CanvasContent | VERIFIED |
| `app/src/renderer/components/timeline/SelectionBox.tsx` | Rubber-band selection | EXISTS | 66 lines, dashed accent border | Rendered in CanvasContent | VERIFIED |
| `app/src/renderer/components/timeline/LoopRegion.tsx` | Loop overlay | EXISTS | 50 lines, 15% opacity fill + brackets | Rendered in CanvasContent | VERIFIED |
| `app/src/renderer/utils/clip-operations.ts` | Clip CRUD functions | EXISTS | 96 lines, 5 functions + engine dispatch | Used by TimelineCanvas, keyboard shortcuts | VERIFIED |
| `app/src/renderer/utils/waveform.ts` | Peak-to-vertex conversion | EXISTS | 45 lines, peaksToVertices + generateMockPeaks | Used by AudioClip | VERIFIED |
| `app/src/renderer/components/shared/ContextMenu.tsx` | Portal-based context menu | EXISTS | 66 lines, createPortal + close-on-outside | Used by TrackHeader, TimelineCanvas | VERIFIED |
| `app/src/renderer/hooks/use-playhead.ts` | rAF engine transport sync | EXISTS | 54 lines, 30Hz polling, graceful fallback | Called in TimelineView | VERIFIED |
| `app/src/renderer/hooks/use-engine-sync.ts` | Engine event listener | EXISTS | 37 lines, onCommandEvent subscription | Called in TimelineView | VERIFIED |
| `app/src/renderer/hooks/use-keyboard-shortcuts.ts` | Global keyboard shortcuts | EXISTS | 108 lines, Delete/Ctrl+Z/Space/S/zoom | Called in TimelineView | VERIFIED |
| `app/src/renderer/hooks/use-context-menu.ts` | Context menu state | EXISTS | 35 lines, show/close state management | Used by TrackHeader, TimelineCanvas | VERIFIED |
| `app/src/renderer/components/tracks/TrackHeader.tsx` | Track header with controls | EXISTS | 188 lines, mute/solo/arm + rename + color | Used by TrackHeaderList | VERIFIED |
| `app/src/renderer/components/tracks/TrackHeaderList.tsx` | Scrollable track list | EXISTS | 20 lines, scroll via translateY | Rendered in TimelineView | VERIFIED |
| `app/src/renderer/components/tracks/AddTrackButton.tsx` | Add Track button | EXISTS | 16 lines, calls addTrack() | Rendered in TimelineView header | VERIFIED |
| `app/src/renderer/components/shared/GridResolutionSelect.tsx` | Grid resolution dropdown | EXISTS | 31 lines, 4 options | Used by TimelineToolbar | VERIFIED |
| `test/beat-math.test.ts` | Beat-math unit tests | EXISTS | 10 tests — all PASSING | vitest run | VERIFIED |
| `test/timeline-store.test.ts` | Store unit tests | EXISTS | 28 tests — all PASSING | vitest run | VERIFIED |
| `test/clip-operations.test.ts` | Clip operations tests | EXISTS | 7 tests — all PASSING | vitest run | VERIFIED |
| `test/waveform-utils.test.ts` | Waveform utility tests | EXISTS | 8 tests — all PASSING | vitest run | VERIFIED |

### Key Link Verification

| From | To | Via | Status | Notes |
|------|----|-----|--------|-------|
| timeline-store.ts | types/timeline.ts | imports Track, Clip, GridResolution, LoopRegion | WIRED | `import type { ..., } from '../types/timeline'` |
| timeline-store.ts | utils/beat-math.ts | snapToBeat in clip ops | REDIRECTED | Plan said store would import snapToBeat; actual implementation puts snap in TimelineCanvas.tsx interactions. Goal (snap works) achieved, wiring location shifted. |
| TimelineView.tsx | timeline-store.ts | useTimelineStore selectors | WIRED | `useTimelineStore(useShallow(...))` for scrollY, tracks |
| TimelineCanvas.tsx | pixi-viewport | Viewport component | WIRED | `import { Viewport } from 'pixi-viewport'`; `extend({ Viewport })` |
| use-playhead.ts | window.calliope.getTransportState | rAF polling | WIRED | `window.calliope.getTransportState()` with graceful fallback |
| ClipContainer.tsx | timeline-store.ts | clips for trackId filter | WIRED | `useTimelineStore((s) => s.clips)` + useMemo filter |
| MidiClip.tsx | utils/colors.ts | track color for clip fill | WIRED | `TRACK_COLORS[trackColorIndex % TRACK_COLORS.length]` |
| clip-operations.ts | window.calliope.dispatchCommand | command dispatch for clip mutations | WIRED | Optional/best-effort; store mutation is primary path (by design per Plan 03) |
| use-engine-sync.ts | window.calliope.onCommandEvent | event listener subscription | WIRED | `window.calliope.onCommandEvent(callback)` |
| TimelineCanvas.tsx | clip-operations.ts | createClip/deleteClip/moveClip on interaction | WIRED | Imported and called in pointer event handlers |
| use-keyboard-shortcuts.ts | window.calliope.commandUndo | Ctrl+Z triggers undo | WIRED | `window.calliope?.commandUndo()` |
| TimelineView.tsx | use-engine-sync.ts | useEngineSync() call | WIRED | Called on mount |
| TimelineView.tsx | use-keyboard-shortcuts.ts | useKeyboardShortcuts() call | WIRED | Called on mount |
| TimelineRuler.tsx | timeline-store.ts setLoopRegion | loop region creation | WIRED | Shift+drag calls `setLoopRegion({ startBeat, endBeat })` |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| Playhead.tsx | currentBeat | use-playhead.ts rAF → getTransportState → setTransportState | Yes — real engine ppqPosition | FLOWING |
| GridLayer.tsx | pixelsPerBeat, gridResolution | Zustand store | Yes — user-mutable state | FLOWING |
| ClipContainer.tsx | clips (filtered) | Zustand store.clips | Yes — addClip populates; no hardcoded empty | FLOWING |
| MidiClip.tsx | clip props | ClipContainer filters from store | Yes — real clip data | FLOWING |
| AudioClip.tsx | clip.peakData | Optional Float32Array; falls back gracefully | Partial — peakData populated externally; waveform falls back to center line | FLOWING (with documented fallback) |
| TrackHeaderList.tsx | tracks | Zustand store.tracks | Yes — addTrack/removeTrack mutate; no hardcoded empty | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| All 53 unit tests pass | `pnpm vitest run test/beat-math.test.ts test/timeline-store.test.ts test/clip-operations.test.ts test/waveform-utils.test.ts` | 4 test files, 53 tests, all PASSED | PASS |
| Build compiles without errors | `cd app && npx electron-vite build` | Build completes in ~10s, 3277 modules transformed | PASS |
| beatToPixel(4, 24, 0) = 96 | Covered by test/beat-math.test.ts | PASSED | PASS |
| snapToBeat(3.3, 0.25) = 3.25 | Covered by test/beat-math.test.ts | PASSED | PASS |
| addTrack creates Track 1 with auto-name | Covered by test/timeline-store.test.ts | PASSED | PASS |
| removeTrack removes all clips on that track | Covered by test/timeline-store.test.ts | PASSED | PASS |

### Requirements Coverage

| Requirement | Source Plan(s) | Description | Status | Evidence |
|-------------|---------------|-------------|--------|---------|
| TL-01 | 06-01, 06-02, 06-04 | Multi-track horizontal timeline with zoomable view and playhead | SATISFIED | TimelineCanvas: wheel zoom via pixelsPerBeat; TrackHeaderList + ClipContainer: multi-track layout; Playhead: CSS div updated by rAF; TimelineRuler: scrolls with canvas |
| TL-02 | 06-01, 06-03, 06-04 | MIDI clips can be created, moved, resized, copied, and deleted | SATISFIED | TimelineCanvas: create via click+drag, move via pointer drag, resize-left/right with edge detection; clip-operations.ts: createClip/moveClip/resizeClip/duplicateClip/deleteClip; keyboard shortcuts: Delete, Ctrl+D |
| TL-03 | 06-03 | Audio clips display waveforms and can be placed on tracks | SATISFIED | AudioClip.tsx: peaksToVertices() renders waveform polygon; clip.type='audio' routes to AudioClip; fallback dashed line when no peakData |
| TL-04 | 06-01, 06-02, 06-04 | Tracks can be added, removed, reordered, renamed, and color-coded | SATISFIED | store: addTrack/removeTrack/renameTrack/reorderTrack/setTrackColor; TrackHeader: double-click rename + context menu with Rename/Change Color/Duplicate/Delete; AddTrackButton: Plus icon button |
| TL-05 | 06-01, 06-02, 06-04 | Snap-to-grid with configurable grid resolution (1/4, 1/8, 1/16, 1/32 notes) | SATISFIED | GridResolutionSelect: 4 grid options; GridLayer: draws grid lines at gridResolution; TimelineCanvas: snapToBeat applied to all drag operations when snapEnabled; Toolbar snap toggle |
| TL-06 | 06-01, 06-03, 06-04 | Loop region selection for repeated playback | SATISFIED | LoopRegion.tsx: accent-colored overlay with bracket lines; TimelineRuler: Shift+drag creates loop region; double-click clears it; dispatches to window.calliope.setLoopRegion |

**All 6 requirements satisfied.** No orphaned requirements found.

### Anti-Patterns Found

| File | Pattern | Severity | Impact |
|------|---------|----------|--------|
| `app/src/renderer/utils/clip-operations.ts` | "Engine clip commands not yet implemented" comment in 5 places | Info | By design — plan explicitly intended store-only path until engine implements clip commands. Dispatch is best-effort with try/catch. Not a blocker. |
| `app/src/renderer/components/timeline/TimelineCanvas.tsx` | SelectionBox rendered with `visible={false}` permanently | Warning | Rubber-band selection is handled via DOM overlay pointer events instead of PixiJS SelectionBox (an architectural shift from Plan 04 spec). Selection still works for clips via pointer events; rubber-band multi-select is partially functional (intersecting clips detected on pointerup). Visual selection box does not animate during drag. |

### Human Verification Required

All 53 automated tests pass and the project builds successfully. The following behaviors require a running Electron process to verify:

**1. Timeline Layout Renders**
- **Test:** `pnpm dev` from Calliope root
- **Expected:** Dark-themed layout with 200px track header column, toolbar, ruler, PixiJS canvas
- **Why human:** Visual rendering requires WebGL in Electron

**2. Clip Creation by Click+Drag**
- **Test:** Click Add Track, then click+drag on the empty canvas area
- **Expected:** Ghost clip appears during drag; releasing creates a named MIDI clip at the dragged beat range
- **Why human:** PixiJS canvas + pointer capture interaction

**3. Clip Selection and Multi-Select**
- **Test:** Click a clip (single select), then Shift+click another
- **Expected:** Selected clips show accent-colored border; Ctrl+click toggles
- **Why human:** Visual selection state on PixiJS canvas

**4. Clip Move and Resize**
- **Test:** Drag a clip to new position; hover near edge to get col-resize cursor; drag edge to resize
- **Expected:** Clip moves/resizes with grid snapping; cursor changes near edges
- **Why human:** Cursor changes + PixiJS canvas interaction

**5. Context Menus**
- **Test:** Right-click track header and right-click a clip
- **Expected:** Dark-themed portal menus with correct items; destructive items are red
- **Why human:** DOM portal rendering in Electron

**6. Keyboard Shortcuts**
- **Test:** Press Delete, Ctrl+Z, Space, S (snap toggle), Ctrl+= (zoom)
- **Expected:** Delete removes selected clips; Space toggles transport; S toggles snap button state
- **Why human:** Keyboard events in Electron renderer

**7. Loop Region on Ruler**
- **Test:** Shift+click+drag on the ruler
- **Expected:** Purple tinted overlay appears on timeline canvas spanning the dragged beat range
- **Why human:** Canvas overlay from PixiJS requires live render

**8. Vertical Scroll Sync**
- **Test:** Add 8+ tracks to overflow, then scroll vertically
- **Expected:** Track headers translateY stays in lockstep with canvas scrollY
- **Why human:** DOM/PixiJS scroll sync requires live app

**9. Zoom via Ctrl+Scroll**
- **Test:** Hold Ctrl and scroll wheel over the canvas
- **Expected:** Grid lines expand/contract; ruler bar spacing changes; clips maintain positions
- **Why human:** WebGL canvas zoom requires live app

### Notable Architectural Observations

1. **pixi-viewport replaced with pixiContainer**: Per the note in the prompt, pixi-viewport's Viewport component was replaced by a plain `pixiContainer` with imperative scroll sync. The Viewport is still imported and extend() registered but the viewport JSX element is not used in the final render. Scroll/zoom is handled via DOM wheel events on the container element, and a PixiJS Container is moved via `scrollContainerRef.current.x/y`. This is a valid fix for the `app.renderer.events` unavailability issue.

2. **SelectionBox stub**: The SelectionBox component is fully implemented (dashed accent rectangle) but is rendered with `visible={false}` permanently. Rubber-band selection works via DOM overlay pointer events (intersecting clips detected on pointerUp). The visual dragging rectangle does not render. This is a warning-level gap — selection functionality works, but the visual rubber-band animation is absent.

3. **Engine clip dispatch is intentionally best-effort**: clip-operations.ts dispatches to `window.calliope?.dispatchCommand` with try/catch. The store is the source of truth for clip state. This is correct — the engine does not yet have clip data model commands (per Plan 03 research notes).

---

_Verified: 2026-03-28T17:03:35Z_
_Verifier: Claude (gsd-verifier)_
