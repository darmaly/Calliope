# Phase 7: Piano Roll - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can edit MIDI notes on a pitch/time grid with velocity control and quantization. This is the second visual UI phase — builds a piano roll editor that opens from MIDI clips created in the Phase 6 timeline. Delivers: note grid with keyboard reference, note drawing/editing/selection, velocity lane with per-note editing, quantize function, scroll/zoom in both axes.

Does NOT deliver: audio recording, mixer view, project save/load, or AI-assisted composition — those are later phases.

</domain>

<decisions>
## Implementation Decisions

### Piano Roll Layout & Rendering
- D-01: Piano roll appears in a collapsible bottom panel with a resizable split divider (drag handle between timeline and piano roll, min/max height constraints)
- D-02: 60px piano keyboard on left edge with note labels on white keys (C-2 to G8), scales with vertical zoom
- D-03: 16px default note row height, vertically zoomable 8-32px via Ctrl+scroll — keys expand/shrink with zoom
- D-04: PixiJS canvas rendering matching Phase 6 patterns (same pixi-setup, beat-math utils, Grid/Container patterns, @pixi/react)

### Note Interactions
- D-05: Click+drag to draw notes (length = drag distance, snapped to grid) — same pattern as clip creation in Phase 6
- D-06: Selection model matches Phase 6: click=select, Shift+click=multi, Ctrl+click=toggle, drag-box=rubber-band
- D-07: Drag note body to move (pitch + time), drag edges to resize, all snapped to grid — mirrors Phase 6 clip interactions
- D-08: Ctrl+D to duplicate (offset right by note length), Ctrl+C/V for clipboard copy/paste

### Velocity Editing
- D-09: Note color opacity indicates velocity (darker = higher velocity) as inline visual cue
- D-10: Collapsible velocity lane below note grid — vertical bar chart per note, drag bars to edit velocity
- D-11: Default velocity for new notes: 100 (out of 127)
- D-12: Multi-note velocity editing: select multiple notes, drag one velocity bar to scale all proportionally

### Quantize & Grid
- D-13: Quantize via toolbar button + Ctrl+Q shortcut, applies to selected notes, 100% strength (full snap)
- D-14: Grid resolution shared with timeline (uses existing gridResolution from Zustand store)
- D-15: Triplet grid subdivisions included: 1/4T, 1/8T, 1/16T alongside straight values (1/4, 1/8, 1/16, 1/32)

### Claude's Discretion
- Component decomposition and file organization within the piano roll
- PixiJS scene graph structure for the note grid
- Keyboard shortcut mappings beyond specified ones
- Note data model structure within clips
- Split panel resize implementation details

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `beat-math.ts` — beatToPixel, pixelToBeat, snapToBeat utilities (direct reuse)
- `colors.ts` — TRACK_COLORS palette (8 hues, reuse for note coloring)
- `pixi-setup.ts` — PixiJS extend registration (Container, Graphics, Text, etc.)
- `timeline-store.ts` — Zustand store with tracks, clips, gridResolution, transport state
- `use-keyboard-shortcuts.ts` — global keyboard hook pattern (extend for piano roll shortcuts)
- `use-playhead.ts` — engine polling for transport position (reuse for piano roll playhead)
- `use-context-menu.ts` — context menu hook + portal rendering
- `ContextMenu.tsx` — shared portal-based context menu component
- `GridResolutionSelect.tsx` — grid resolution dropdown (extend with triplet options)
- `TimelineCanvas.tsx` — drag state machine pattern (DragMode enum, pointer lifecycle)

### Established Patterns
- Beat-based positioning: all coordinates in beats, converted to pixels via pixelsPerBeat
- Zustand + useShallow selectors for targeted re-renders
- Imperative PixiJS container sync (bypass React reconciler for scroll/transform)
- Optimistic UI: store update first, engine dispatch second with try-catch
- Drag state machine: pointer down → determine mode → pointer move → update → pointer up → finalize
- Dark theme: #1a1a2e background, #252542 surfaces, #6c63ff accent
- 80px track height, 72px clip height in timeline

### Integration Points
- Clip model already has `type: 'midi'` — piano roll edits notes WITHIN a selected MIDI clip
- `window.calliope.dispatchCommand()` for engine operations
- `gridResolution` in timeline store needs triplet values added (GridResolution type extension)
- App.tsx layout needs split panel with piano roll below timeline
- Timeline clip double-click or selection should open piano roll for that clip

</code_context>

<specifics>
## Specific Ideas

- Resizable split panel between timeline and piano roll (user-requested)
- Vertical zoom on piano roll to expand/shrink note rows and keyboard (user-requested)
- Triplet grid subdivisions (1/4T, 1/8T, 1/16T) alongside straight values (user-requested)
- Reuse Phase 6 interaction patterns (drag state machine, selection model, grid snapping) for consistency

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
