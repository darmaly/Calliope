# Phase 6: Timeline & Arrangement - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can arrange music on a multi-track horizontal timeline with clips, tracks, and grid snapping. This is the first visual UI phase — builds the React/PixiJS timeline view on top of the C++ engine from Phases 1-5.

Delivers: multi-track timeline with horizontal scrolling/zoom and moving playhead, MIDI clip creation/move/resize/copy/delete, audio clip display with waveforms, track management (add/remove/reorder/rename/color), and snap-to-grid with configurable resolution.

Does NOT deliver: piano roll editor, mixer view, or project save/load — those are Phases 7-9.

</domain>

<decisions>
## Implementation Decisions

### Timeline Layout & Rendering
- D-01: Timeline canvas renders via PixiJS/WebGL for GPU-accelerated performance with thousands of clips/notes
- D-02: Fixed left panel (200px) for track headers — track name + controls always visible, standard DAW convention
- D-03: Playhead is a vertical line overlay spanning all tracks, CSS-positioned div updated via requestAnimationFrame, synced to engine transport position
- D-04: Horizontal zoom via scroll wheel + Ctrl/Cmd, vertical zoom separate, snap grid scales with zoom level

### Clip Interactions & Track Management
- D-05: Clips created by click+drag on empty track area — draws from start to end of drag region, snapped to grid
- D-06: Standard multi-selection: click to select, Shift+click for multi, Ctrl+click to toggle, drag-box for rubber-band selection
- D-07: Track management via right-click context menu on track header (add/remove/rename/reorder/color) plus "+" button at bottom for new tracks
- D-08: Clip resize by dragging left/right edge handles, cursor changes to resize icon, snaps to grid

### State Management & Engine Integration
- D-09: Zustand store for timeline state (tracks, clips, transport, zoom) with selector subscriptions to prevent unnecessary re-renders
- D-10: Command dispatcher for mutations (UI dispatches commands), event listener for state updates (listens via onCommandEvent)
- D-11: Clip data model uses beat-based positioning (startBeat, lengthBeats, trackId, type) for musical grid alignment
- D-12: Grid resolution stored in Zustand, controls both snap behavior and visual grid lines — user selects 1/4, 1/8, 1/16, 1/32 from dropdown

### Claude's Discretion
- Component decomposition and file organization
- PixiJS scene graph structure for the timeline canvas
- Waveform rendering approach for audio clips (pre-computed peaks vs. live rendering)
- Keyboard shortcut mappings for common operations

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `window.calliope` API (app/src/preload/index.ts) — all engine commands accessible from renderer
- `onCommandEvent` callback — real-time event stream from C++ engine
- CommandDispatcher — all mutations flow through commands for undo/redo
- Zustand (installed in package.json) — state management ready to use
- PixiJS (specified in CLAUDE.md) — needs installation
- Tailwind CSS (specified in CLAUDE.md) — needs installation for non-canvas UI

### Established Patterns
- IPC via contextBridge (contextIsolation: true)
- Command dispatch: `window.calliope.dispatchCommand({command, params})`
- Undo/redo: `window.calliope.commandUndo()` / `commandRedo()`
- State query: `window.calliope.getProjectState()`

### Integration Points
- app/src/renderer/ — React components go here
- app/src/preload/index.ts — extend with new timeline-specific APIs if needed
- engine/ — may need track/clip data model commands on C++ side
- package.json — add PixiJS, @pixi/react, Tailwind CSS, wavesurfer.js

</code_context>

<specifics>
## Specific Ideas

- Beat-based clip positioning aligns naturally with the musical grid and transport BPM
- PixiJS handles the performance-critical canvas (clips, waveforms, grid lines, playhead)
- Tailwind handles the chrome (track headers, toolbars, context menus)
- Zustand selectors prevent re-rendering the entire timeline when one clip moves

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 06-timeline-arrangement*
*Context gathered: 2026-03-28*
