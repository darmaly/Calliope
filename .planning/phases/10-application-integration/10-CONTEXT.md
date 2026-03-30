# Phase 10: Application Integration - Context

**Gathered:** 2026-03-29
**Status:** Ready for planning

<domain>
## Phase Boundary

All panels unite into a cohesive DAW workspace with professional navigation, rendering, and shortcuts. This phase polishes the existing layout, adds a transport bar, renders audio clip waveforms with PixiJS/WebGL, and implements global keyboard shortcuts.

</domain>

<decisions>
## Implementation Decisions

### Workspace Layout & Navigation
- Panel arrangement is a vertical stack: timeline top, piano roll/mixer bottom — refine and polish the existing layout, do not redesign
- Panel switching via click-to-toggle buttons in toolbar (unify existing Phase 7-8 toggle patterns into consistent panel bar)
- Drag-to-resize dividers between all panels using existing SplitDivider pattern from Phase 6
- Instrument/effect panels integrated into mixer channel strips (Phase 8) — no separate panels needed

### Transport Bar & Rendering
- Transport bar is a fixed top bar above the timeline — always visible, standard DAW position
- Transport controls: Play/Stop/Record buttons, BPM display, time signature, playhead position in both MM:SS.ms and bars:beats format
- Audio clip waveforms render via PixiJS/WebGL on the timeline — GPU-accelerated per success criteria
- Playhead synced via CSS overlay at ~60fps using requestAnimationFrame (Phase 6 established pattern)

### Keyboard Shortcuts & Polish
- Standard DAW shortcut conventions: Space=play/stop, R=record, Ctrl+Z/Y=undo/redo, Ctrl+S=save, Delete=remove selected
- Active panel gets shortcut priority (piano roll > timeline > global), using capture-phase listener pattern from Phase 7
- Click-to-focus panels with subtle border highlight as visual focus indicator

### Claude's Discretion
- Transport bar exact pixel dimensions and styling
- BPM display edit interaction (click to type vs scroll)
- Waveform peak data generation and caching strategy
- Panel minimum/maximum sizes for resize constraints
- Focus border color and animation

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `SplitDivider` component — drag-to-resize between panels (Phase 6)
- `TimelineToolbar.tsx` — existing toggle buttons for Piano Roll and Mixer
- CSS playhead overlay pattern from Phase 6
- Capture-phase keydown listener from Phase 7 (`stopImmediatePropagation` for panel priority)
- `window.calliope.transport*` IPC methods — play, stop, record, get position
- PixiJS setup and `@pixi/react` integration from Phase 6 timeline
- `useTimelineStore` — transport state (playing, recording, bpm, position)
- Phase 6 waveform rendering pattern (closed polygon approach for flat coordinate arrays)

### Established Patterns
- Zustand stores with selectors for granular re-renders
- Portal-based popovers and context menus
- Pointer events for all interactions
- Tailwind CSS v4 for non-canvas UI
- Canvas 2D for simple animations (meters), PixiJS for complex 2D (timeline clips)

### Integration Points
- App.tsx: add TransportBar component above timeline
- TimelineToolbar.tsx: unify panel toggle buttons into consistent bar
- Timeline clips: replace solid color rectangles with PixiJS waveform rendering
- Global keyboard handler: document-level keydown listener with panel focus routing
- Window title: integrate project name from project-store

</code_context>

<specifics>
## Specific Ideas

No specific requirements — standard DAW integration patterns.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
