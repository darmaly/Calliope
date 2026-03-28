# Phase 7: Piano Roll - Research

**Researched:** 2026-03-28
**Domain:** PixiJS-based MIDI piano roll editor with note editing, velocity control, and quantization
**Confidence:** HIGH

## Summary

Phase 7 builds a piano roll MIDI editor that opens from MIDI clips created in the Phase 6 timeline. The implementation is predominantly a UI phase that follows established patterns from Phase 6 -- same PixiJS rendering approach, same Zustand state management, same drag state machine interaction pattern, same dark theme. The technical risk is low because every core pattern (beat-math coordinate conversion, pointer event handling, grid rendering, context menus, keyboard shortcuts) already exists and has been proven in Phase 6.

The primary new challenges are: (1) a split panel layout to show the piano roll below the timeline, (2) a 128-row pitch grid with a piano keyboard reference, (3) per-note velocity editing with a collapsible velocity lane, and (4) extending the GridResolution type with triplet subdivisions. None of these require new dependencies -- they are all achievable with the existing PixiJS 8 + @pixi/react + Zustand + Tailwind stack.

**Primary recommendation:** Mirror Phase 6 patterns exactly. Create a new `piano-roll-store.ts` Zustand store for piano roll state, build PixiJS canvas components for the note grid and keyboard, and use the same transparent DOM overlay + pointer event pattern for interactions. Extend the existing `GridResolution` type to include triplet values.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- D-01: Piano roll appears in a collapsible bottom panel with a resizable split divider (drag handle between timeline and piano roll, min/max height constraints)
- D-02: 60px piano keyboard on left edge with note labels on white keys (C-2 to G8), scales with vertical zoom
- D-03: 16px default note row height, vertically zoomable 8-32px via Ctrl+scroll -- keys expand/shrink with zoom
- D-04: PixiJS canvas rendering matching Phase 6 patterns (same pixi-setup, beat-math utils, Grid/Container patterns, @pixi/react)
- D-05: Click+drag to draw notes (length = drag distance, snapped to grid) -- same pattern as clip creation in Phase 6
- D-06: Selection model matches Phase 6: click=select, Shift+click=multi, Ctrl+click=toggle, drag-box=rubber-band
- D-07: Drag note body to move (pitch + time), drag edges to resize, all snapped to grid -- mirrors Phase 6 clip interactions
- D-08: Ctrl+D to duplicate (offset right by note length), Ctrl+C/V for clipboard copy/paste
- D-09: Note color opacity indicates velocity (darker = higher velocity) as inline visual cue
- D-10: Collapsible velocity lane below note grid -- vertical bar chart per note, drag bars to edit velocity
- D-11: Default velocity for new notes: 100 (out of 127)
- D-12: Multi-note velocity editing: select multiple notes, drag one velocity bar to scale all proportionally
- D-13: Quantize via toolbar button + Ctrl+Q shortcut, applies to selected notes, 100% strength (full snap)
- D-14: Grid resolution shared with timeline (uses existing gridResolution from Zustand store)
- D-15: Triplet grid subdivisions included: 1/4T, 1/8T, 1/16T alongside straight values (1/4, 1/8, 1/16, 1/32)

### Claude's Discretion
- Component decomposition and file organization within the piano roll
- PixiJS scene graph structure for the note grid
- Keyboard shortcut mappings beyond specified ones
- Note data model structure within clips
- Split panel resize implementation details

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PR-01 | Piano roll editor displays MIDI notes on a pitch/time grid with velocity | PixiJS canvas with 128-row pitch grid, NoteRect components with velocity-modulated alpha, piano keyboard reference. New piano-roll-store with MidiNote model. |
| PR-02 | Notes can be drawn, selected, moved, resized, copied, and deleted | Drag state machine pattern from TimelineCanvas.tsx, pointer event overlay, same edge detection for resize. Note operations module mirroring clip-operations.ts. |
| PR-03 | Velocity editing per note via velocity lane or note color | Velocity lane as collapsible PixiJS sub-canvas with vertical bars. Note alpha formula: 0.2 + (velocity/127) * 0.8. Multi-select proportional scaling. |
| PR-04 | Quantize function snaps selected notes to grid resolution | snapToBeat utility already exists. Quantize applies to all selectedNoteIds, snapping startBeat to gridResolution. |
| PR-05 | Piano roll supports scroll, zoom, and keyboard reference on left edge | Independent scrollX/scrollY in piano-roll-store. Vertical zoom (noteRowHeight 8-32px). 60px piano keyboard column scaling with zoom. Same wheel event handler pattern as TimelineCanvas. |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **Rendering:** PixiJS 8 with @pixi/react for all canvas rendering (piano roll grid, notes, keyboard)
- **State management:** Zustand (v5 with `create<>`) for piano roll store
- **Styling:** Tailwind CSS v4 via @tailwindcss/vite plugin for DOM elements (toolbar, panels)
- **Icons:** lucide-react for toolbar icons
- **No external component libraries** -- all custom built
- **IPC pattern:** `window.calliope.dispatchCommand()` for engine operations, try/catch wrapper
- **Dark theme:** #1a1a2e background, #252542 surfaces, #6c63ff accent, #3a3a5a borders
- **Font:** Inter, sans-serif

## Standard Stack

### Core (already installed -- no new dependencies)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| pixi.js | ^8.17.1 | WebGL note grid, keyboard, velocity lane rendering | Already in use for Phase 6 timeline |
| @pixi/react | ^8.0.5 | React integration for PixiJS components | Already in use |
| zustand | ^5.0.0 | Piano roll state store | Already in use for timeline store |
| react | ^19.0.0 | UI framework | Already in use |
| tailwindcss | ^4.2.2 | DOM element styling (toolbar, split panel) | Already in use |
| lucide-react | ^1.7.0 | Toolbar icons | Already in use |

### No New Dependencies Required

This phase requires zero new npm packages. All rendering, state management, and interaction patterns are covered by the existing stack.

## Architecture Patterns

### Recommended Project Structure
```
app/src/renderer/
  components/
    piano-roll/
      PianoRollPanel.tsx        # Split panel container + divider
      PianoRollToolbar.tsx       # Quantize, velocity toggle, zoom controls
      PianoRollCanvas.tsx        # PixiJS Application + pointer overlay (mirrors TimelineCanvas)
      PianoKeyboard.tsx          # 60px left-edge keyboard (PixiJS Graphics)
      NoteGrid.tsx               # Background grid with pitch rows + beat columns (PixiJS)
      NoteRect.tsx               # Single MIDI note rectangle (PixiJS, mirrors MidiClip)
      VelocityLane.tsx           # Collapsible velocity bar chart (PixiJS)
      SplitDivider.tsx           # Drag handle between timeline and piano roll (DOM)
  stores/
    piano-roll-store.ts          # Zustand store for piano roll state
  types/
    piano-roll.ts                # MidiNote interface, PianoRollState types
  utils/
    note-operations.ts           # CRUD functions mirroring clip-operations.ts
    piano-helpers.ts             # Pitch-to-name, pitch-to-Y, isBlackKey utilities
```

### Pattern 1: Separate Zustand Store (not extending timeline-store)
**What:** Create a dedicated `piano-roll-store.ts` rather than adding note state to the timeline store.
**When to use:** When a new editor has its own viewport state (scroll, zoom), selection model, and data.
**Why:** The piano roll has independent scrollX/scrollY, its own selection set (notes not clips), and a separate zoom axis (noteRowHeight). Mixing these into the timeline store would create coupling and unnecessary re-renders.

```typescript
// piano-roll-store.ts
import { create } from 'zustand'

interface MidiNote {
  id: string
  pitch: number       // 0-127
  startBeat: number
  lengthBeats: number
  velocity: number    // 1-127
}

interface PianoRollState {
  activeClipId: string | null
  notes: Record<string, MidiNote>
  selectedNoteIds: Set<string>
  noteRowHeight: number      // 8-32, default 16
  scrollX: number
  scrollY: number            // default ~centered on C4
  velocityLaneVisible: boolean
  velocityLaneHeight: number // default 80
  panelHeight: number        // split panel height, default 300
  clipboard: MidiNote[]
}
```

### Pattern 2: Transparent DOM Overlay for Pointer Events
**What:** A `<div className="absolute inset-0">` sits on top of the PixiJS `<Application>` and handles all pointer events. PixiJS handles rendering only.
**Why:** This is the proven Phase 6 pattern. It avoids PixiJS event propagation issues and gives full React control over the interaction state machine.

```typescript
// PianoRollCanvas.tsx structure (mirrors TimelineCanvas.tsx)
<div className="relative w-full h-full">
  <Application resizeTo={containerRef} background="#1a1a2e" antialias>
    <CanvasContent containerRef={containerRef} />
  </Application>
  <div
    className="absolute inset-0"
    style={{ zIndex: 10 }}
    onPointerDown={handlePointerDown}
    onPointerMove={handlePointerMove}
    onPointerUp={handlePointerUp}
    onContextMenu={handleContextMenu}
  />
  {/* Ghost note preview, context menu portals */}
</div>
```

### Pattern 3: Drag State Machine
**What:** A `DragMode` enum + `DragState` object tracks the current interaction mode. Pointer down determines mode, pointer move updates, pointer up finalizes.
**Why:** Proven in TimelineCanvas. Same modes needed for piano roll: 'none' | 'create' | 'select' | 'move' | 'resize-left' | 'resize-right' | 'velocity'.

### Pattern 4: Imperative Scroll Sync
**What:** Subscribe to Zustand store changes and imperatively update a PixiJS Container's x/y position, bypassing React reconciliation.
**Why:** Scroll performance. React re-renders on every scroll pixel are too slow for smooth scrolling. Phase 6 uses this pattern with `useTimelineStore.subscribe()`.

```typescript
useEffect(() => {
  const unsub = usePianoRollStore.subscribe((state) => {
    if (scrollContainerRef.current) {
      scrollContainerRef.current.x = -state.scrollX
      scrollContainerRef.current.y = -state.scrollY
    }
  })
  return unsub
}, [])
```

### Pattern 5: Note Hit Testing
**What:** Convert pointer coordinates to beat + pitch, then search notes for a match. Similar to `findClipAt` in TimelineCanvas.
**Why:** Needed for click selection, drag initiation, and edge detection for resize.

```typescript
function findNoteAt(px: number, py: number): NoteHit | null {
  const state = usePianoRollStore.getState()
  const beat = pixelToBeat(px, timelineStore.pixelsPerBeat, state.scrollX)
  const pitch = Math.floor((py + state.scrollY) / state.noteRowHeight)
  const invertedPitch = 127 - pitch // top of canvas = pitch 127

  for (const note of Object.values(state.notes)) {
    if (note.pitch !== invertedPitch) continue
    if (beat >= note.startBeat && beat <= note.startBeat + note.lengthBeats) {
      // Check edge proximity for resize
      const leftPx = beatToPixel(note.startBeat, ppb, state.scrollX)
      const rightPx = beatToPixel(note.startBeat + note.lengthBeats, ppb, state.scrollX)
      let edge: 'left' | 'right' | 'body' = 'body'
      if (Math.abs(px - leftPx) <= 6) edge = 'left'
      else if (Math.abs(px - rightPx) <= 6) edge = 'right'
      return { note, edge }
    }
  }
  return null
}
```

### Pattern 6: Split Panel with Drag Resize
**What:** The App.tsx layout changes from `<TimelineView />` to a vertical split layout with a drag divider between timeline and piano roll.
**Why:** D-01 requires a resizable split panel.

```typescript
// App.tsx layout change
<div className="flex flex-col h-screen">
  <div style={{ flex: `1 1 0`, minHeight: 200 }}>
    <TimelineView />
  </div>
  <SplitDivider onDrag={handleDividerDrag} onDoubleClick={toggleCollapse} />
  <div style={{ height: panelHeight, minHeight: collapsed ? 36 : 200 }}>
    <PianoRollPanel />
  </div>
</div>
```

### Anti-Patterns to Avoid
- **Sharing scroll state between timeline and piano roll:** They have independent viewports. Only `gridResolution` and `pixelsPerBeat` are shared (from timeline store).
- **Using PixiJS events for interaction:** Use the DOM overlay pattern. PixiJS event handling has quirks with overlapping elements and doesn't integrate well with React state.
- **Re-rendering PixiJS containers via React props for scroll:** Use imperative Container.x/y updates via store subscription.
- **Adding note data to the Clip type in timeline.ts:** Notes belong in the piano-roll-store. The Clip model stays lightweight. The piano roll loads notes for the `activeClipId`.
- **Building velocity lane as a separate PixiJS Application:** Keep it in the same PixiJS canvas, positioned below the note grid area. One Application per panel.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Beat-to-pixel conversion | Custom math | `beat-math.ts` (beatToPixel, pixelToBeat, snapToBeat) | Already proven, handles scroll offset |
| Context menu | Custom dropdown | `ContextMenu.tsx` + `useContextMenu` hook | Portal-based, auto-dismiss, dark themed |
| Grid resolution picker | Custom select | `GridResolutionSelect.tsx` (extend with triplets) | Already styled, wired to store |
| Keyboard shortcuts | Raw event listeners | `useKeyboardShortcuts.ts` (extend) | Handles input field skipping, modifier keys |
| PixiJS setup | Manual extend calls | `pixi-setup.ts` (import it) | Already registers Container, Graphics, Text, etc. |

**Key insight:** Phase 6 built all the interaction and rendering infrastructure. Phase 7 reuses it with different data (notes instead of clips, pitch rows instead of track rows).

## Common Pitfalls

### Pitfall 1: Pitch Inversion
**What goes wrong:** MIDI pitch 0 is the lowest note, but in screen coordinates Y=0 is the top. If you map pitch directly to Y, the keyboard appears upside-down.
**Why it happens:** Natural to map pitch to Y without inverting.
**How to avoid:** Use `invertedRow = 127 - pitch` for Y positioning. Pitch 127 (G8) is row 0 at the top, pitch 0 (C-2) is row 127 at the bottom.
**Warning signs:** Piano keyboard shows C-2 at the top instead of G8.

### Pitfall 2: Scroll Desynchronization Between Keyboard and Grid
**What goes wrong:** The piano keyboard scrolls at a different rate than the note grid, so key labels don't align with note rows.
**Why it happens:** Keyboard and grid are in different containers with separate scroll handling.
**How to avoid:** Both the keyboard and the grid must share the same scrollY and noteRowHeight values. The keyboard should be part of the same scroll container, or synced imperatively to the same Y offset.
**Warning signs:** Notes appear to be on the wrong pitch when scrolling.

### Pitfall 3: Triplet Grid Resolution Floating Point
**What goes wrong:** Triplet values (1/3 of a beat = 0.33333...) cause floating point accumulation errors in grid line positioning and snap calculations.
**Why it happens:** 1/3 cannot be represented exactly in IEEE 754 floating point.
**How to avoid:** Use high-precision fraction values (0.16667, 0.08333, 0.04167) and apply epsilon tolerance (1e-6) in grid line "is this a bar boundary" checks. The existing `snapToBeat` using `Math.round` handles this acceptably.
**Warning signs:** Grid lines drift visually after many subdivisions; notes snap to slightly wrong positions.

### Pitfall 4: Velocity Lane Alignment with Scrolling Notes
**What goes wrong:** Velocity bars don't stay vertically aligned with their notes during horizontal scroll.
**Why it happens:** The velocity lane and note grid scroll independently or the velocity lane doesn't share scrollX.
**How to avoid:** Velocity lane must share scrollX from the piano-roll store and use the same pixelsPerBeat for positioning bars.
**Warning signs:** Velocity bars appear to shift relative to notes during scroll.

### Pitfall 5: Large Note Count Performance
**What goes wrong:** Rendering 1000+ notes causes frame drops.
**Why it happens:** Each NoteRect as a separate PixiJS Graphics object with individual draw callbacks.
**How to avoid:** Use `cullable={true}` on note containers (established Phase 6 pattern). For extreme cases, batch note rendering into a single Graphics draw call rather than individual components. Start with individual components and optimize only if needed.
**Warning signs:** FPS drops below 30 when zoomed out with many notes visible.

### Pitfall 6: Split Panel Resize Fights with Content
**What goes wrong:** Dragging the split divider causes layout thrashing or content overflow.
**Why it happens:** Using CSS flexbox without proper min/max constraints, or not preventing text selection during drag.
**How to avoid:** Use `user-select: none` on the body during divider drag. Use `min-height` constraints (200px min for piano roll, ensure timeline has minimum too). Use `requestAnimationFrame` throttling on the drag handler.
**Warning signs:** Flickering during resize, content overflows panel bounds.

## Code Examples

### Piano Key Helper Utilities
```typescript
// piano-helpers.ts
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const BLACK_KEY_INDICES = new Set([1, 3, 6, 8, 10]) // C#, D#, F#, G#, A#

export function pitchToNoteName(pitch: number): string {
  const octave = Math.floor(pitch / 12) - 2 // MIDI 0 = C-2
  const note = NOTE_NAMES[pitch % 12]
  return `${note}${octave}`
}

export function isBlackKey(pitch: number): boolean {
  return BLACK_KEY_INDICES.has(pitch % 12)
}

export function pitchToY(pitch: number, noteRowHeight: number): number {
  return (127 - pitch) * noteRowHeight
}

export function yToPitch(y: number, noteRowHeight: number, scrollY: number): number {
  return 127 - Math.floor((y + scrollY) / noteRowHeight)
}

export function velocityToAlpha(velocity: number): number {
  return 0.2 + (velocity / 127) * 0.8
}
```

### GridResolution Type Extension for Triplets (D-15)
```typescript
// types/timeline.ts -- extend the union type
export type GridResolution =
  | 0.25 | 0.125 | 0.0625 | 0.03125           // straight: 1/4, 1/8, 1/16, 1/32
  | 0.16667 | 0.08333 | 0.04167                // triplet: 1/4T, 1/8T, 1/16T

// GridResolutionSelect options extension
const GRID_OPTIONS: { label: string; value: GridResolution }[] = [
  { label: '1/4', value: 0.25 },
  { label: '1/4T', value: 0.16667 },
  { label: '1/8', value: 0.125 },
  { label: '1/8T', value: 0.08333 },
  { label: '1/16', value: 0.0625 },
  { label: '1/16T', value: 0.04167 },
  { label: '1/32', value: 0.03125 },
]
```

### Quantize Implementation
```typescript
// note-operations.ts
export function quantizeSelectedNotes(): void {
  const prStore = usePianoRollStore.getState()
  const gridRes = useTimelineStore.getState().gridResolution

  if (prStore.selectedNoteIds.size === 0) return

  const updatedNotes = { ...prStore.notes }
  for (const noteId of prStore.selectedNoteIds) {
    const note = updatedNotes[noteId]
    if (note) {
      updatedNotes[noteId] = {
        ...note,
        startBeat: snapToBeat(note.startBeat, gridRes),
      }
    }
  }
  usePianoRollStore.setState({ notes: updatedNotes })
}
```

### Velocity Bar Proportional Scaling (D-12)
```typescript
function handleVelocityDrag(draggedNoteId: string, newVelocity: number): void {
  const state = usePianoRollStore.getState()
  const original = state.notes[draggedNoteId]
  if (!original) return

  const ratio = newVelocity / original.velocity
  const updatedNotes = { ...state.notes }

  for (const noteId of state.selectedNoteIds) {
    const note = updatedNotes[noteId]
    if (note) {
      updatedNotes[noteId] = {
        ...note,
        velocity: Math.max(1, Math.min(127, Math.round(note.velocity * ratio))),
      }
    }
  }
  usePianoRollStore.setState({ notes: updatedNotes })
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| PixiJS 7 event system | PixiJS 8 with DOM overlay for events | Project Phase 6 | Proven pattern, do not use PixiJS events |
| pixi-viewport for scroll/zoom | Manual wheel handler + imperative Container transform | Project Phase 6 | More control, no viewport dependency for piano roll |
| Redux for state | Zustand v5 with `create<>` | Project standard | Simpler API, selector subscriptions |

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 3.x |
| Config file | `/Users/deebarmaly/Calliope/vitest.config.ts` |
| Quick run command | `vitest run test/piano-roll-store.test.ts` |
| Full suite command | `vitest run` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PR-01 | Piano roll store holds notes with pitch/time/velocity | unit | `vitest run test/piano-roll-store.test.ts -t "note model"` | Wave 0 |
| PR-01 | Piano helpers (pitchToNoteName, isBlackKey, velocityToAlpha) | unit | `vitest run test/piano-helpers.test.ts` | Wave 0 |
| PR-02 | Note CRUD operations (add, remove, move, resize, duplicate) | unit | `vitest run test/note-operations.test.ts` | Wave 0 |
| PR-02 | Selection model (select, multi-select, toggle, clear) | unit | `vitest run test/piano-roll-store.test.ts -t "selection"` | Wave 0 |
| PR-03 | Velocity editing (single note, proportional multi-note) | unit | `vitest run test/note-operations.test.ts -t "velocity"` | Wave 0 |
| PR-04 | Quantize snaps notes to grid resolution | unit | `vitest run test/note-operations.test.ts -t "quantize"` | Wave 0 |
| PR-05 | Scroll/zoom state management | unit | `vitest run test/piano-roll-store.test.ts -t "scroll"` | Wave 0 |
| PR-05 | GridResolution type accepts triplet values | unit | `vitest run test/beat-math.test.ts -t "triplet"` | Wave 0 |

### Sampling Rate
- **Per task commit:** `vitest run test/piano-roll-store.test.ts test/note-operations.test.ts test/piano-helpers.test.ts`
- **Per wave merge:** `vitest run`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `test/piano-roll-store.test.ts` -- covers PR-01, PR-02 (selection), PR-05 (scroll/zoom state)
- [ ] `test/note-operations.test.ts` -- covers PR-02 (CRUD), PR-03 (velocity), PR-04 (quantize)
- [ ] `test/piano-helpers.test.ts` -- covers PR-01 (pitch names, black key detection, velocity alpha)
- [ ] Extend `test/beat-math.test.ts` with triplet snap tests -- covers PR-05

## Open Questions

1. **Note persistence across clip switches**
   - What we know: Notes live in `piano-roll-store.notes` keyed by `activeClipId`. When user opens a different clip, notes need to be saved and new clip's notes loaded.
   - What's unclear: Where to persist notes when switching clips -- should `Clip` in timeline store gain a `notes` field, or should piano-roll-store maintain a `Record<clipId, Record<noteId, MidiNote>>` cache?
   - Recommendation: Add a `notes?: Record<string, MidiNote>` field to the `Clip` type. On clip open, load into piano-roll store. On clip close or deactivation, flush back to the clip record. This keeps data with its owner and makes project serialization easier in Phase 9.

2. **pixelsPerBeat sharing between timeline and piano roll**
   - What we know: D-14 says grid resolution is shared. The horizontal zoom (pixelsPerBeat) is stored in timeline-store.
   - What's unclear: Should horizontal zoom be independent or shared?
   - Recommendation: Share pixelsPerBeat from timeline store for horizontal zoom. Piano roll reads it from `useTimelineStore`. Vertical zoom (noteRowHeight) is independent in piano-roll-store. This matches user expectation that zoom level is consistent across views.

## Sources

### Primary (HIGH confidence)
- Phase 6 codebase: `TimelineCanvas.tsx`, `timeline-store.ts`, `beat-math.ts`, `clip-operations.ts`, `MidiClip.tsx`, `GridLayer.tsx` -- all patterns verified by reading source
- Phase 7 UI Spec: `07-UI-SPEC.md` -- approved design contract with dimensions, colors, interaction contract
- Phase 7 CONTEXT.md -- locked decisions D-01 through D-15

### Secondary (MEDIUM confidence)
- MIDI standard pitch mapping (0=C-2, 60=C4, 127=G8) -- well-established industry standard
- Velocity alpha formula from UI spec: `0.2 + (velocity/127) * 0.8`

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- zero new dependencies, all patterns proven in Phase 6
- Architecture: HIGH -- direct mirror of established TimelineCanvas/timeline-store patterns
- Pitfalls: HIGH -- based on direct code analysis of Phase 6 implementation and DAW domain knowledge

**Research date:** 2026-03-28
**Valid until:** 2026-04-28 (stable -- no external dependency changes expected)
