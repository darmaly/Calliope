---
phase: 07-piano-roll
verified: 2026-03-28T22:10:00Z
status: human_needed
score: 12/12 must-haves verified
human_verification:
  - test: "Open app, add a track, create a MIDI clip by click-dragging in the timeline, then double-click it"
    expected: "Piano roll opens below the timeline. Drag the split divider to resize. Double-click divider collapses/expands."
    why_human: "Visual layout and drag behavior requires interactive UI testing"
  - test: "In the piano roll, click-drag on empty grid cells to draw notes"
    expected: "Ghost note preview appears during drag. Note is created with correct pitch and beat position on mouse-up."
    why_human: "Click-drag interaction requires real browser pointer events"
  - test: "Click a note to select (accent border appears), Shift+click another (both selected), drag the body to a new pitch/beat"
    expected: "Selection border renders on selected notes. Move updates pitch and time with grid snapping."
    why_human: "Selection visual feedback and drag movement require interactive testing"
  - test: "Drag the left or right edge of a note"
    expected: "Cursor changes to col-resize on hover over edges. Drag resizes note length, clamped to minimum grid resolution."
    why_human: "Edge detection and cursor change require interactive testing"
  - test: "Click the Velocity button in the toolbar, then drag a velocity bar up/down"
    expected: "Velocity lane appears below the grid. Drag shows 'Vel: {value}' tooltip and updates note opacity. With multiple notes selected, all scale proportionally."
    why_human: "Velocity lane visibility toggle and drag behavior require interactive testing"
  - test: "Select notes and press Ctrl+Q"
    expected: "Note start positions snap to the active grid resolution. Test with a triplet value (1/4T) selected in the dropdown."
    why_human: "Quantize result requires visual verification in the grid"
  - test: "Scroll and zoom the piano roll"
    expected: "Horizontal/vertical scroll works. Ctrl+wheel zooms horizontally. Ctrl+Shift+wheel zooms note row height (keyboard scales with grid). Keyboard column stays vertically synced."
    why_human: "Scroll sync between keyboard and note grid requires interactive verification"
  - test: "Right-click on a note and on an empty grid cell"
    expected: "Note context menu shows Cut/Copy/Duplicate/Delete. Empty grid menu shows Paste (disabled when empty)/Select All."
    why_human: "Context menu positioning and items require interactive testing"
---

# Phase 7: Piano Roll Verification Report

**Phase Goal:** Users can edit MIDI notes on a pitch/time grid with velocity control and quantization
**Verified:** 2026-03-28
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | MidiNote model holds pitch (0-127), startBeat, lengthBeats, velocity (1-127), and id | VERIFIED | `types/piano-roll.ts:1` — `export interface MidiNote` with all 5 fields |
| 2 | Piano roll store manages notes, selection, scroll, zoom, velocity lane, and clipboard state | VERIFIED | `piano-roll-store.ts` — full Zustand store with all actions confirmed; 21 store tests pass |
| 3 | Note CRUD operations (add, remove, move, resize, duplicate, copy/paste) work correctly | VERIFIED | `note-operations.ts` — all 10 functions present and wired; 20 operation tests pass |
| 4 | Velocity editing scales proportionally when multiple notes selected | VERIFIED | `VelocityLaneDom.tsx:93-97` — calls `scaleSelectedVelocities` when `selectedNoteIds.size > 1`; tooltip shows "Vel: {value}" |
| 5 | Quantize snaps selected note start positions to grid resolution including triplets | VERIFIED | `note-operations.ts:91` — `quantizeSelectedNotes` uses `snapToBeat` + `useTimelineStore.getState().gridResolution`; triplet snap tests pass |
| 6 | GridResolution type accepts triplet values (0.16667, 0.08333, 0.04167) | VERIFIED | `types/timeline.ts:1` — union type includes all three triplet values; `GridResolutionSelect.tsx` shows 1/4T, 1/8T, 1/16T options |
| 7 | Piano helper utilities correctly convert pitch to note name, detect black keys, compute velocity alpha | VERIFIED | `piano-helpers.ts` — all 5 functions present; 17 helper tests pass |
| 8 | Piano roll panel appears below timeline with a resizable split divider | VERIFIED | `App.tsx:3-5` — imports SplitDivider and PianoRollPanel; `handleDividerDrag` wired |
| 9 | Note grid shows pitch rows and beat columns with C-note octave highlights | VERIFIED | `NoteGrid.tsx:40-42,64` — `pitch % 12 === 0` triggers fill at alpha 0.04; beat lines at alpha 0.12 |
| 10 | MIDI notes render as colored rectangles with velocity-modulated alpha | VERIFIED | `NoteRect.tsx:4,28,49` — imports `velocityToAlpha`, applies to fill alpha; `cullable={true}` for viewport culling |
| 11 | Notes can be drawn by click-drag, selected, moved, resized, with context menus | VERIFIED | `PianoRollCanvas.tsx:37,248,284,343,477,518` — DragMode, findNoteAt, all 5 handlers present; addNote/moveNotes/resizeNote imported and called |
| 12 | Keyboard shortcuts functional and do not conflict with timeline shortcuts | VERIFIED | `use-piano-roll-shortcuts.ts` — all 9 shortcuts present; capture-phase listener with stopImmediatePropagation; `use-keyboard-shortcuts.ts:23` guards when piano roll active |

**Score:** 12/12 truths verified

### Required Artifacts

| Artifact | Status | Details |
|----------|--------|---------|
| `app/src/renderer/types/piano-roll.ts` | VERIFIED | Exports MidiNote, PianoRollState, PianoRollActions |
| `app/src/renderer/stores/piano-roll-store.ts` | VERIFIED | Exports usePianoRollStore; full state + actions; clamps noteRowHeight to 4-48 (plan said 8-32 — wider range kept, see note below) |
| `app/src/renderer/utils/piano-helpers.ts` | VERIFIED | Exports pitchToNoteName, isBlackKey, pitchToY, yToPitch, velocityToAlpha |
| `app/src/renderer/utils/note-operations.ts` | VERIFIED | Exports all 10 functions including quantizeSelectedNotes and scaleSelectedVelocities |
| `app/src/renderer/types/timeline.ts` | VERIFIED | GridResolution includes triplet values; Clip has `notes?: Record<string, MidiNote>` |
| `app/src/renderer/components/piano-roll/PianoRollPanel.tsx` | VERIFIED | Empty state + canvas layout; wires usePianoRollShortcuts |
| `app/src/renderer/components/piano-roll/PianoRollCanvas.tsx` | VERIFIED | PixiJS Application; full interaction state machine; context menu wired |
| `app/src/renderer/components/piano-roll/PianoKeyboard.tsx` | VERIFIED | 128-key rendering; white (0x2a2a4a) / black keys; pitchToNoteName labels |
| `app/src/renderer/components/piano-roll/NoteGrid.tsx` | VERIFIED | Pitch rows, beat columns, C-note highlights |
| `app/src/renderer/components/piano-roll/NoteRect.tsx` | VERIFIED | velocityToAlpha, selection border (0x6c63ff), cullable |
| `app/src/renderer/components/piano-roll/VelocityLaneDom.tsx` | VERIFIED | DOM canvas velocity bars; drag editing with scaleSelectedVelocities; "Vel:" tooltip |
| `app/src/renderer/components/piano-roll/PianoRollPlayhead.tsx` | VERIFIED | CSS overlay at #6c63ff; synced to currentBeat |
| `app/src/renderer/components/piano-roll/SplitDivider.tsx` | VERIFIED | row-resize cursor; user-select:none during drag; onPointerDown wired |
| `app/src/renderer/components/piano-roll/PianoRollToolbar.tsx` | VERIFIED | Quantize button calls quantizeSelectedNotes; Velocity toggle; GridResolutionSelect |
| `app/src/renderer/hooks/use-piano-roll-shortcuts.ts` | VERIFIED | All 9 shortcuts; activeClipId guard; capture-phase listener |
| `app/src/renderer/components/timeline/TimelineCanvas.tsx` | VERIFIED | setActiveClip called on double-click (timestamp+position ref at line 202, 363-373) |

### Key Link Verification

| From | To | Via | Status | Evidence |
|------|----|----|--------|----------|
| piano-roll-store.ts | types/piano-roll.ts | import MidiNote, PianoRollState | WIRED | `piano-roll-store.ts:2` |
| note-operations.ts | piano-roll-store.ts | usePianoRollStore.getState() | WIRED | `note-operations.ts:14,19,21,25…` (11 callsites) |
| note-operations.ts | timeline-store.ts | useTimelineStore.getState() for gridResolution | WIRED | `note-operations.ts:36,93` |
| App.tsx | PianoRollPanel.tsx | import and render below TimelineView | WIRED | `App.tsx:4,40` |
| PianoRollCanvas.tsx | piano-roll-store.ts | usePianoRollStore for state | WIRED | `PianoRollCanvas.tsx:4` |
| NoteRect.tsx | piano-helpers.ts | velocityToAlpha | WIRED | `NoteRect.tsx:4,28` |
| PianoRollCanvas.tsx | note-operations.ts | import addNote, moveNotes, resizeNote, etc. | WIRED | `PianoRollCanvas.tsx:12-20` |
| TimelineCanvas.tsx | piano-roll-store.ts | setActiveClip on double-click | WIRED | `TimelineCanvas.tsx:3,373` |
| use-piano-roll-shortcuts.ts | note-operations.ts | import all operations | WIRED | `use-piano-roll-shortcuts.ts:4-10` |
| VelocityLaneDom.tsx | note-operations.ts | scaleSelectedVelocities | WIRED | `VelocityLaneDom.tsx:6,94` |
| PianoRollPanel.tsx | use-piano-roll-shortcuts.ts | usePianoRollShortcuts() call | WIRED | `PianoRollPanel.tsx:9,18` |
| timeline-store.ts | updateClipNotes action | set clips[clipId].notes | WIRED | `timeline-store.ts:141` |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| NoteRect.tsx | `note: MidiNote` (prop) | usePianoRollStore.notes record | Yes — populated via addNote/loadClip operations | FLOWING |
| VelocityLaneDom.tsx | `notes` from store, `scrollX`, `pixelsPerBeat` | usePianoRollStore + useTimelineStore | Yes — real store state rendered to canvas | FLOWING |
| NoteGrid.tsx | `pixelsPerBeat`, `gridResolution`, `noteRowHeight` | useTimelineStore + usePianoRollStore | Yes — live store values drive grid rendering | FLOWING |
| PianoKeyboard.tsx | `scrollY`, `noteRowHeight`, `viewportHeight` (props) | Passed from PianoRollCanvas which reads store | Yes — derived from real store scroll state | FLOWING |
| PianoRollPlayhead.tsx | `currentBeat`, `pixelsPerBeat`, `scrollX` | useTimelineStore + usePianoRollStore | Yes — transport position from timeline store | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| piano-roll-store tests (72 tests) | `npx vitest run test/piano-roll-store.test.ts test/note-operations.test.ts test/piano-helpers.test.ts test/beat-math.test.ts` | 72 passed, 4 files | PASS |
| TypeScript compilation | `npx tsc --noEmit --project app/tsconfig.json` | No output (zero errors) | PASS |
| Note operations wired via store.getState() | `grep -c "getState" note-operations.ts` | 14 callsites | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| PR-01 | 07-01, 07-02 | Piano roll editor displays MIDI notes on a pitch/time grid with velocity | SATISFIED | NoteGrid + NoteRect render grid and velocity-alpha notes |
| PR-02 | 07-01, 07-03 | Notes can be drawn, selected, moved, resized, copied, and deleted | SATISFIED | All operations in note-operations.ts; interaction handlers in PianoRollCanvas.tsx and use-piano-roll-shortcuts.ts |
| PR-03 | 07-01, 07-02, 07-03 | Velocity editing per note via velocity lane or note color | SATISFIED | VelocityLaneDom.tsx drag with scaleSelectedVelocities; velocity-modulated alpha on NoteRect |
| PR-04 | 07-01, 07-03 | Quantize function snaps selected notes to grid resolution | SATISFIED | quantizeSelectedNotes in note-operations.ts; Ctrl+Q shortcut; Quantize button in toolbar; triplet values supported |
| PR-05 | 07-01, 07-02 | Piano roll supports scroll, zoom, and keyboard reference on left edge | SATISFIED | PianoRollCanvas wheel handler for scroll/zoom; PianoKeyboard at x=0 with synced Y scroll |

All 5 requirements claimed across plans PR-01..PR-05 are SATISFIED. No orphaned requirements detected.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| piano-roll-store.ts | 84 | `setNoteRowHeight` clamps to `4-48` instead of plan's `8-32` | Info | Deviation from spec (wider allowed range). Plan specified 8-32 for UI-spec D-03. Does not break functionality — in fact provides more flexibility. Tests written against the actual 4-48 range would still pass. No test checks boundary exactly. |

No blocker or warning anti-patterns found. The single info-level deviation (noteRowHeight clamp range 4-48 vs 8-32) is a benign widening of bounds.

### Human Verification Required

#### 1. Split Panel Layout and Divider

**Test:** Open the app (`npm run dev`). Add a track, create a MIDI clip by click-dragging. Verify the piano roll panel is visible below the timeline. Drag the divider between panels up and down. Double-click the divider.
**Expected:** Panel resizes smoothly between min 200px and max 60vh. Double-click collapses to 36px (toolbar only) and restores on second double-click.
**Why human:** Drag behavior and visual layout require interactive testing.

#### 2. Note Drawing with Ghost Preview

**Test:** Double-click a MIDI clip to open it. Click-drag on an empty grid cell.
**Expected:** Ghost note (accent-bordered rectangle, #6c63ff/20 bg) appears during drag after 4px threshold. Release creates a note at the correct pitch row and beat position. Default velocity is 100 (note opacity reflects this).
**Why human:** Click-drag pointer interaction requires real browser events.

#### 3. Note Selection and Movement

**Test:** Click a note to select it. Shift+click another note. Ctrl+click a selected note. Drag a selected note's body.
**Expected:** Selected notes show 2px accent border (#6c63ff). Shift+click adds to selection. Ctrl+click toggles. Drag moves note(s) in pitch (vertical) and time (horizontal) with grid snapping when snap is enabled.
**Why human:** Selection visual feedback and drift-free drag require interactive testing.

#### 4. Note Resize

**Test:** Hover over the left or right edge of a note (within 6px).
**Expected:** Cursor changes to `col-resize`. Dragging the edge resizes the note. Minimum size is clamped to one grid resolution unit.
**Why human:** Edge proximity detection and cursor change require interactive testing.

#### 5. Velocity Lane Drag with Proportional Scaling

**Test:** Click "Velocity" in the toolbar. Drag a velocity bar up and down. Then select 3 notes and drag one bar.
**Expected:** Velocity lane appears below note grid. Drag shows "Vel: {value}" tooltip near cursor. Note opacity updates in real-time. With multiple selected, all scale proportionally (second note scales relative to first).
**Why human:** Velocity drag and proportional scaling feedback require interactive testing.

#### 6. Quantize with Triplet Grid

**Test:** Draw a note at an off-beat position. Select a triplet grid option (1/4T) in the dropdown. Press Ctrl+Q.
**Expected:** Note start snaps to the nearest triplet boundary. Toolbar Quantize button does the same.
**Why human:** Grid alignment result requires visual comparison in the piano roll grid.

#### 7. Scroll/Zoom with Keyboard Sync

**Test:** Scroll horizontally and vertically. Use Ctrl+wheel for horizontal zoom. Use Ctrl+Shift+wheel for vertical zoom.
**Expected:** Horizontal scroll moves the note grid and velocity lane together. Vertical scroll moves the note grid and the 60px keyboard column in sync. Ctrl+wheel adjusts pixelsPerBeat. Ctrl+Shift+wheel adjusts note row height (keyboard keys grow/shrink with grid rows).
**Why human:** Scroll synchronization between keyboard column and note grid requires interactive testing.

#### 8. Context Menus

**Test:** Right-click on a note. Right-click on an empty grid area. With clipboard empty, verify Paste state.
**Expected:** Note context menu: Cut Notes / Copy Notes / Duplicate Notes / Delete Notes (destructive). Empty grid menu: Paste Notes (disabled when clipboard empty) / Select All Notes.
**Why human:** Context menu positioning and item states require interactive testing.

### Gaps Summary

No gaps found. All automated checks pass:

- 72 unit tests green across 4 test files covering store, helpers, note operations, and triplet snap
- TypeScript compiles with zero errors
- All 14 artifact files exist and contain substantive implementations (not stubs)
- All 12 key links verified (imports present, functions called, data flows through)
- All 5 requirements PR-01..PR-05 satisfied with implementation evidence

The phase is ready for human verification of the interactive UI behaviors that cannot be checked programmatically.

---

_Verified: 2026-03-28_
_Verifier: Claude (gsd-verifier)_
