# Phase 8: Mixer - Context

**Gathered:** 2026-03-29
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can mix tracks with volume, pan, mute/solo, effect inserts, and real-time level meters. The mixer provides a dedicated panel with per-track channel strips and a master strip, integrating with the existing audio engine's InsertChain and effect processing infrastructure from Phase 5.

</domain>

<decisions>
## Implementation Decisions

### Mixer Layout & Visual Design
- Mixer appears as a separate tab/panel, toggled from a toolbar button (consistent with Piano Roll toggle pattern)
- Channel strips arranged vertically left-to-right, scrollable horizontally
- Faders rendered as DOM-based vertical range sliders with custom Tailwind styling
- Level meters rendered with Canvas 2D animated bars (green/yellow/red gradient)

### Controls & Interactions
- Pan control is a rotary knob using CSS rotation with pointer drag, displaying -100L to +100R
- Effect insert UI uses dropdown slot list per strip with add/remove/reorder/bypass controls
- Volume range is -inf to +6dB with dB markings on the fader scale
- Master strip is rightmost, visually wider, always visible, with its own insert chain

### Engine Integration & State
- Level meter data polled from C++ engine via IPC at ~30Hz
- State management extends existing timeline-store (Track type gains volume/pan fields; mute/solo already present)
- Effect parameter editing via click-to-open popover with parameter sliders

### Claude's Discretion
- Specific CSS styling choices for faders, knobs, and meters
- Animation easing and smoothing for meter decay
- Exact pixel dimensions of channel strips and master strip
- Scroll behavior and overflow handling for many tracks

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `timeline-store.ts` — Zustand store with Track type (id, name, colorIndex, muted, solo, armed, order) and toggle actions
- `TrackHeader.tsx` — Existing M/S/Arm button pattern (amber mute, green solo, red arm)
- `InsertChain` (C++) — Double-buffer atomic pointer swap for real-time-safe effect chain modifications
- Effect processors: CompressorProcessor, ParametricEQProcessor, ReverbProcessor, DelayProcessor, LimiterProcessor
- `window.calliope.effectInsert/Remove/Reorder/Bypass` — IPC bridge already exposes effect commands
- `MasterBusProcessor` — `std::atomic<float> masterVolume` for master volume control
- `AudioGraph` — Routing: instruments → per-track InsertChainProcessors → master InsertChainProcessor → output

### Established Patterns
- Zustand v5 `create<>` with `Set` for selection state and `crypto.randomUUID()` for IDs
- `command:dispatch` IPC path handles all commands without per-feature endpoints
- ThreadSafeFunction + Promise for non-blocking bridge calls
- Tailwind CSS v4 via @tailwindcss/vite plugin (no config file)
- Portal-based context menus with document click listener for dismissal
- Pointer events (not mouse events) for all interaction handlers
- Canvas 2D for ruler rendering (Phase 6 precedent for simple animated canvas)

### Integration Points
- Toolbar: add mixer toggle button alongside existing Piano Roll toggle
- Timeline store: extend Track type with volume (number) and pan (number) fields
- Bridge: add meter data subscription endpoint; volume/pan parameter commands
- AudioGraph: track volume/pan enforcement at C++ level (mute/solo/volume/pan applied before InsertChain)
- Parameter registry: volume/pan use scheme `track.{trackId}.volume` and `track.{trackId}.pan`

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches following DAW conventions.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
