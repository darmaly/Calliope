# Phase 6: Timeline & Arrangement - Research

**Researched:** 2026-03-28
**Domain:** React/PixiJS timeline UI, Zustand state management, Tailwind CSS styling
**Confidence:** HIGH

## Summary

This is the first frontend/UI phase. The codebase is currently 100% backend (C++/JUCE engine + Electron shell with a placeholder React app). This phase introduces PixiJS 8 canvas rendering, Tailwind CSS 4 styling, and Zustand state management to build a multi-track horizontal timeline -- the foundational DAW view.

The core technical challenge is synchronizing three rendering domains: a PixiJS WebGL canvas for high-performance clip/grid rendering, DOM-based React components for track headers and toolbars, and a CSS-positioned playhead overlay driven by `requestAnimationFrame`. These three layers must share scroll/zoom state and stay visually synchronized. Zustand v5 (already installed) provides the shared state mechanism with selector subscriptions to prevent unnecessary re-renders.

**Primary recommendation:** Use `@pixi/react` v8 with `extend()` for the canvas layer, `pixi-viewport` for scroll/zoom management inside the canvas, custom PixiJS Graphics for clip rendering, PixiJS Mesh line mode for waveform peaks, Tailwind CSS 4 via `@tailwindcss/vite` plugin for DOM chrome, and Zustand v5 with `useShallow` for multi-value selectors.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- D-01: Timeline canvas renders via PixiJS/WebGL for GPU-accelerated performance with thousands of clips/notes
- D-02: Fixed left panel (200px) for track headers -- track name + controls always visible, standard DAW convention
- D-03: Playhead is a vertical line overlay spanning all tracks, CSS-positioned div updated via requestAnimationFrame, synced to engine transport position
- D-04: Horizontal zoom via scroll wheel + Ctrl/Cmd, vertical zoom separate, snap grid scales with zoom level
- D-05: Clips created by click+drag on empty track area -- draws from start to end of drag region, snapped to grid
- D-06: Standard multi-selection: click to select, Shift+click for multi, Ctrl+click to toggle, drag-box for rubber-band selection
- D-07: Track management via right-click context menu on track header (add/remove/rename/reorder/color) plus "+" button at bottom for new tracks
- D-08: Clip resize by dragging left/right edge handles, cursor changes to resize icon, snaps to grid
- D-09: Zustand store for timeline state (tracks, clips, transport, zoom) with selector subscriptions to prevent unnecessary re-renders
- D-10: Command dispatcher for mutations (UI dispatches commands), event listener for state updates (listens via onCommandEvent)
- D-11: Clip data model uses beat-based positioning (startBeat, lengthBeats, trackId, type) for musical grid alignment
- D-12: Grid resolution stored in Zustand, controls both snap behavior and visual grid lines -- user selects 1/4, 1/8, 1/16, 1/32 from dropdown

### Claude's Discretion
- Component decomposition and file organization
- PixiJS scene graph structure for the timeline canvas
- Waveform rendering approach for audio clips (pre-computed peaks vs. live rendering)
- Keyboard shortcut mappings for common operations

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TL-01 | Multi-track horizontal timeline with zoomable view and playhead | PixiJS 8 canvas + pixi-viewport for scroll/zoom + CSS playhead overlay with rAF sync to engine transport |
| TL-02 | MIDI clips can be created, moved, resized, copied, and deleted on tracks | PixiJS Graphics for clip rects with hit areas, Zustand actions dispatching commands via window.calliope.dispatchCommand |
| TL-03 | Audio clips display waveforms and can be placed on tracks | Pre-computed peaks from C++ engine, rendered via PixiJS Mesh line mode for performance |
| TL-04 | Tracks can be added, removed, reordered, renamed, and color-coded | DOM-based track headers (React + Tailwind), right-click context menu, Zustand track state |
| TL-05 | Snap-to-grid with configurable grid resolution (1/4, 1/8, 1/16, 1/32 notes) | Grid resolution in Zustand, snap function converts pixel position to nearest beat subdivision |
| TL-06 | Loop region selection for repeated playback of a section | Ruler interaction (Shift+click+drag), dispatches transport:setLoopRegion via existing preload API |
</phase_requirements>

## Standard Stack

### Core (to install)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| pixi.js | 8.17.1 | WebGL canvas rendering for timeline | GPU-accelerated 2D, handles thousands of clips at 60fps. Locked by CLAUDE.md and D-01. |
| @pixi/react | 8.0.5 | React-PixiJS integration | Official React 19 + PixiJS 8 bindings. Thin JSX wrapper using `extend()` API. |
| pixi-viewport | 6.0.3 | Scroll/zoom/pan for canvas | PixiJS 8 compatible. Provides drag, wheel zoom, decelerate, clamp -- exactly what a timeline viewport needs. |
| tailwindcss | 4.2.2 | Utility CSS for DOM components | Locked by CLAUDE.md. v4 uses Vite plugin, no PostCSS config needed. |
| @tailwindcss/vite | 4.2.2 | Tailwind Vite integration | First-party Vite plugin for Tailwind v4. Zero-config, auto-detects templates. |
| wavesurfer.js | 7.12.5 | Waveform visualization (optional/deferred) | CLAUDE.md specifies it, but for this phase waveforms render inside PixiJS canvas via Mesh lines from pre-computed peaks. wavesurfer.js may be more useful in Phase 10 for standalone audio preview. |
| lucide-react | 1.7.0 | Icon library | Lightweight, tree-shakable, MIT. Specified in UI-SPEC. |

### Already Installed

| Library | Version | Purpose | Notes |
|---------|---------|---------|-------|
| zustand | 5.0.12 | State management | Already in package.json. NOTE: v5, not v4.5 as CLAUDE.md states. Use `useShallow` for multi-value selectors. |
| react | 19.x | UI framework | Already installed |
| react-dom | 19.x | React DOM | Already installed |
| typescript | 5.7+ | Type safety | Already installed |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| pixi-viewport | Manual scroll/zoom via Container transforms | pixi-viewport handles wheel events, clamp, decelerate, and edge cases. Manual implementation would be 200+ lines of brittle scroll math. Use pixi-viewport. |
| wavesurfer.js for clip waveforms | PixiJS Mesh line mode | wavesurfer.js renders into its own DOM element/canvas, cannot render inside a PixiJS stage. For in-canvas waveform clips, use PixiJS Mesh with pre-computed peak data. Reserve wavesurfer.js for standalone audio preview contexts. |
| Custom context menu component | @radix-ui/react-context-menu | No shadcn/radix initialized. Custom is simpler for the 8-item menus needed. No library needed. |

**Installation:**
```bash
pnpm add pixi.js @pixi/react pixi-viewport lucide-react
pnpm add -D tailwindcss @tailwindcss/vite
```

**Note on wavesurfer.js:** Do NOT install wavesurfer.js in this phase. Audio clip waveforms render inside the PixiJS canvas using Mesh line mode from pre-computed Float32Array peaks. wavesurfer.js creates its own DOM canvas element and cannot be embedded inside a PixiJS stage. Install it later if needed for standalone waveform preview.

## Architecture Patterns

### Recommended Project Structure
```
app/src/renderer/
├── main.tsx                    # React entry point (exists)
├── App.tsx                     # Root component (replace placeholder)
├── App.css                     # Global styles + Tailwind import
├── stores/
│   └── timeline-store.ts       # Zustand store for timeline state
├── components/
│   ├── timeline/
│   │   ├── TimelineView.tsx        # Top-level layout (headers + canvas + ruler)
│   │   ├── TimelineCanvas.tsx      # PixiJS Application + Viewport wrapper
│   │   ├── TimelineRuler.tsx       # DOM-based bar/beat ruler
│   │   ├── TimelineToolbar.tsx     # Grid, snap, zoom controls
│   │   ├── Playhead.tsx            # CSS-positioned div, rAF updates
│   │   ├── GridLayer.tsx           # PixiJS grid lines component
│   │   ├── ClipContainer.tsx       # PixiJS container per track row
│   │   ├── MidiClip.tsx            # PixiJS MIDI clip rendering
│   │   ├── AudioClip.tsx           # PixiJS audio clip with waveform mesh
│   │   ├── SelectionBox.tsx        # Rubber-band selection rectangle
│   │   └── LoopRegion.tsx          # Loop overlay on ruler/canvas
│   ├── tracks/
│   │   ├── TrackHeaderList.tsx     # Scrollable column of track headers
│   │   ├── TrackHeader.tsx         # Track name, color strip, mute/solo/arm
│   │   └── AddTrackButton.tsx      # "+" button below track list
│   └── shared/
│       ├── ContextMenu.tsx         # Reusable right-click menu component
│       └── GridResolutionSelect.tsx # Grid resolution dropdown
├── hooks/
│   ├── use-engine-sync.ts         # Sync engine events to Zustand store
│   ├── use-playhead.ts            # rAF loop reading engine transport position
│   ├── use-keyboard-shortcuts.ts  # Global keyboard shortcut handler
│   └── use-context-menu.ts        # Right-click menu state/positioning
├── utils/
│   ├── beat-math.ts               # Beat/pixel conversion, snap-to-grid
│   ├── clip-operations.ts         # Clip CRUD dispatching to engine
│   └── colors.ts                  # Track color palette constants
└── types/
    ├── calliope.d.ts              # Existing API types (extend for Phase 6)
    └── timeline.ts                # Timeline-specific type definitions
```

### Pattern 1: PixiJS Scene Graph via @pixi/react v8

**What:** Use `extend()` to register PixiJS classes, then render them as JSX elements with `pixi` prefix.
**When to use:** All canvas rendering in the timeline.

```typescript
// Source: https://github.com/pixijs/pixi-react/blob/main/README.md
import { Application, extend, useApplication } from '@pixi/react'
import { Container, Graphics, Mesh } from 'pixi.js'
import { Viewport } from 'pixi-viewport'

// Register components once at module level
extend({ Container, Graphics, Mesh, Viewport })

// Declare TypeScript types for custom components
declare module '@pixi/react' {
  interface PixiElements {
    viewport: PixiReactElementProps<typeof Viewport>
  }
}

function TimelineCanvas() {
  return (
    <Application resizeTo={canvasContainerRef}>
      <pixiViewport
        screenWidth={width}
        screenHeight={height}
        worldWidth={worldWidth}
        worldHeight={worldHeight}
      >
        <GridLayer />
        {tracks.map(track => (
          <ClipContainer key={track.id} track={track} />
        ))}
      </pixiViewport>
    </Application>
  )
}
```

### Pattern 2: Zustand v5 Store with useShallow

**What:** Zustand v5 requires `useShallow` for multi-value selectors to prevent infinite re-render loops.
**When to use:** Any component selecting multiple values from the store.

```typescript
// Source: https://github.com/pmndrs/zustand/blob/main/docs/migrations/migrating-to-v5.md
import { create } from 'zustand'
import { useShallow } from 'zustand/shallow'

const useTimelineStore = create<TimelineState>((set, get) => ({
  tracks: [],
  clips: {},
  pixelsPerBeat: 24,
  gridResolution: 0.25, // 1/4 note
  snapEnabled: true,
  selectedClipIds: new Set<string>(),
  // ... actions
  addTrack: (track) => set((state) => ({
    tracks: [...state.tracks, track]
  })),
  selectClip: (id, multi) => set((state) => {
    const next = multi ? new Set(state.selectedClipIds) : new Set<string>()
    next.add(id)
    return { selectedClipIds: next }
  }),
}))

// CORRECT: Single value selector (no useShallow needed)
const pixelsPerBeat = useTimelineStore(s => s.pixelsPerBeat)

// CORRECT: Multi-value selector with useShallow
const { tracks, clips } = useTimelineStore(
  useShallow(s => ({ tracks: s.tracks, clips: s.clips }))
)

// WRONG in v5: Returns new reference each render, causes infinite loop
// const [a, b] = useTimelineStore(s => [s.a, s.b])
```

### Pattern 3: DOM + Canvas Scroll Synchronization

**What:** Track headers (DOM) and timeline canvas (PixiJS) share vertical scroll position.
**When to use:** The core layout pattern for the timeline view.

```typescript
// Store scroll position in Zustand
// DOM track header list reads scrollY and sets scrollTop
// PixiJS viewport reads scrollY and sets container.y
// Changes from either source update Zustand, which triggers the other

function TimelineView() {
  const scrollY = useTimelineStore(s => s.scrollY)
  const headerListRef = useRef<HTMLDivElement>(null)

  // Sync DOM scroll to store
  useEffect(() => {
    if (headerListRef.current) {
      headerListRef.current.scrollTop = scrollY
    }
  }, [scrollY])

  return (
    <div className="flex h-full">
      <div
        ref={headerListRef}
        className="w-[200px] overflow-y-hidden"
        onScroll={(e) => useTimelineStore.setState({ scrollY: e.currentTarget.scrollTop })}
      >
        <TrackHeaderList />
      </div>
      <TimelineCanvas />
    </div>
  )
}
```

### Pattern 4: Command Dispatch via Existing Bridge

**What:** All mutations flow through `window.calliope.dispatchCommand()` for undo/redo support.
**When to use:** Every clip/track operation.

```typescript
// Existing preload API (from app/src/preload/index.ts)
async function createClip(trackId: string, startBeat: number, lengthBeats: number, type: 'midi' | 'audio') {
  await window.calliope.dispatchCommand({
    command: 'clip.create',
    params: { trackId, startBeat, lengthBeats, type }
  })
}

// Listen for engine state changes
window.calliope.onCommandEvent((event) => {
  if (event.type === 'stateChanged') {
    // Parse event.data and update Zustand store
    const state = JSON.parse(event.data)
    useTimelineStore.setState({
      tracks: state.tracks,
      clips: state.clips,
    })
  }
})
```

### Pattern 5: Playhead via requestAnimationFrame

**What:** CSS-positioned div updated at 60fps from engine transport position.
**When to use:** Playhead rendering (D-03).

```typescript
function usePlayhead() {
  const [position, setPosition] = useState(0)
  const pixelsPerBeat = useTimelineStore(s => s.pixelsPerBeat)
  const scrollX = useTimelineStore(s => s.scrollX)

  useEffect(() => {
    let animId: number
    const update = async () => {
      const state = await window.calliope.getTransportState()
      setPosition(state.ppqPosition * pixelsPerBeat - scrollX)
      animId = requestAnimationFrame(update)
    }
    animId = requestAnimationFrame(update)
    return () => cancelAnimationFrame(animId)
  }, [pixelsPerBeat, scrollX])

  return position
}
```

### Pattern 6: Waveform Rendering via PixiJS Mesh

**What:** Render audio clip waveforms using Mesh line mode instead of Graphics for performance.
**When to use:** Audio clip waveform display (TL-03).

```typescript
// Pre-computed peaks from C++ engine as Float32Array
// Convert peaks to vertex positions for Mesh line rendering
function createWaveformGeometry(peaks: Float32Array, width: number, height: number) {
  const points: number[] = []
  const step = width / peaks.length
  for (let i = 0; i < peaks.length; i++) {
    points.push(i * step, height / 2 - peaks[i] * height / 2)
  }
  // Use as Mesh geometry with PIXI.MeshGeometry
  return points
}
```

### Anti-Patterns to Avoid

- **Graphics.clear() every frame for waveforms:** Graphics rebuild is expensive. Use Mesh for waveforms, swap pre-built GraphicsContext for dynamic content.
- **Selecting multiple Zustand values without useShallow in v5:** Returns new reference each render, causes infinite re-render loop. Always use `useShallow` for object/array selectors.
- **Using wavesurfer.js inside PixiJS canvas:** wavesurfer.js creates its own DOM canvas, cannot be embedded in a PixiJS stage. Render waveform peaks directly in PixiJS.
- **Synchronous IPC for playhead position:** Never use `ipcRenderer.sendSync`. Use async `invoke` with rAF batching.
- **Re-rendering entire clip list when one clip moves:** Use Zustand selectors scoped to individual clips. Each clip component subscribes only to its own data.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Canvas scroll/zoom/pan | Custom wheel handler + transform math | `pixi-viewport` | Handles deceleration, clamping, edge bounce, zoom-to-cursor. Manual implementation would be 300+ lines and miss edge cases. |
| PixiJS-React bridge | Custom reconciler | `@pixi/react` v8 | Thin wrapper, auto-tracks PixiJS API changes, handles lifecycle correctly. |
| Viewport culling | Manual visible-rect calculation | PixiJS `cullable` property | Set `cullable = true` on clip containers. PixiJS handles culling automatically for off-screen objects. |
| UUID generation | Custom ID function | `crypto.randomUUID()` | Built into all modern browsers/Electron. No library needed. |

## Common Pitfalls

### Pitfall 1: Zustand v5 Infinite Re-render Loop
**What goes wrong:** Components using multi-value selectors like `useStore(s => [s.a, s.b])` cause "Maximum update depth exceeded" errors.
**Why it happens:** Zustand v5 dropped default shallow comparison for selectors. Array/object selectors return new references each render.
**How to avoid:** Always use `useShallow` from `zustand/shallow` for multi-value selectors. Single-value primitive selectors are fine without it.
**Warning signs:** "Maximum update depth exceeded" error in console.

### Pitfall 2: PixiJS Application Not Cleaning Up
**What goes wrong:** Memory leaks, WebGL context errors, or stale event listeners when components unmount.
**Why it happens:** PixiJS `Application` holds GPU resources that React unmount does not automatically release.
**How to avoid:** `@pixi/react` `<Application>` component handles cleanup. Never instantiate `PIXI.Application` manually in a React component.
**Warning signs:** WebGL context lost warnings, increasing memory usage.

### Pitfall 3: Scroll Desync Between DOM and Canvas
**What goes wrong:** Track headers scroll independently from the canvas, creating visual misalignment.
**Why it happens:** DOM scroll events and PixiJS viewport scroll are separate systems with different timing.
**How to avoid:** Use Zustand as the single source of truth for scroll position. DOM `onScroll` and viewport `moved` events both write to Zustand. Both read from Zustand via selectors. Use `overflow-y: hidden` on the DOM list and programmatically set `scrollTop`.
**Warning signs:** Track headers drift from their corresponding canvas rows during fast scrolling.

### Pitfall 4: Waveform Rendering Performance
**What goes wrong:** Audio clips with Graphics-based waveforms cause frame drops during scroll/zoom.
**Why it happens:** PixiJS Graphics rebuild on every frame when cleared/redrawn. For 1000+ line segments per waveform, this is too expensive.
**How to avoid:** Use PixiJS Mesh in line drawing mode for waveforms. Vertices Buffer renders lines without Graphics overhead. Pre-compute peak data in C++ engine, pass as Float32Array.
**Warning signs:** Frame rate drops below 30fps when scrolling through audio clips.

### Pitfall 5: Tailwind CSS Not Working in electron-vite
**What goes wrong:** Tailwind classes have no effect, all components appear unstyled.
**Why it happens:** Tailwind CSS v4 requires the `@tailwindcss/vite` plugin in the Vite config. electron-vite has a separate `renderer` config section.
**How to avoid:** Add `tailwindcss()` to the `renderer.plugins` array in `electron.vite.config.ts`. Add `@import "tailwindcss"` to the main CSS file. No tailwind.config.js needed in v4.
**Warning signs:** Tailwind classes in JSX produce no visual effect.

### Pitfall 6: pixi-viewport Events Conflicting with DOM Events
**What goes wrong:** Scroll wheel events are captured by pixi-viewport, preventing DOM scroll or zoom shortcuts.
**Why it happens:** pixi-viewport attaches global wheel listeners that `preventDefault()`.
**How to avoid:** Configure pixi-viewport with `passiveWheel: false` and explicitly handle Ctrl+wheel for zoom vs plain wheel for scroll. Use `interaction` option to scope event handling to the canvas element only.
**Warning signs:** Ctrl+scroll zoom works but regular scroll is captured by the canvas.

### Pitfall 7: @pixi/react extend() Called Inside Component
**What goes wrong:** PixiJS components fail to render or throw "component not found" errors.
**Why it happens:** `extend()` must be called at module scope (top level), not inside a React component render cycle.
**How to avoid:** Call `extend({ Container, Graphics, Mesh, ... })` once at the top of the file or in a shared setup file.
**Warning signs:** Console errors about unknown pixi elements.

## Code Examples

### Tailwind CSS v4 Setup in electron-vite

```typescript
// app/electron.vite.config.ts
import { defineConfig, externalizeDepsPlugin } from 'electron-vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  main: {
    plugins: [externalizeDepsPlugin()],
    build: {
      rollupOptions: {
        external: [/\.node$/]
      }
    }
  },
  preload: {
    plugins: [externalizeDepsPlugin()]
  },
  renderer: {
    plugins: [react(), tailwindcss()],
  }
})
```

```css
/* app/src/renderer/App.css */
@import "tailwindcss";
```

### Beat-to-Pixel Conversion Utility

```typescript
// Source: DAW standard pattern
export function beatToPixel(beat: number, pixelsPerBeat: number, scrollX: number): number {
  return beat * pixelsPerBeat - scrollX
}

export function pixelToBeat(pixel: number, pixelsPerBeat: number, scrollX: number): number {
  return (pixel + scrollX) / pixelsPerBeat
}

export function snapToBeat(beat: number, gridResolution: number): number {
  return Math.round(beat / gridResolution) * gridResolution
}
```

### PixiJS + pixi-viewport TypeScript Setup

```typescript
// app/src/renderer/components/timeline/pixi-setup.ts
import { extend } from '@pixi/react'
import { Container, Graphics, Mesh, Text, MeshGeometry, MeshMaterial } from 'pixi.js'
import { Viewport } from 'pixi-viewport'

// Register all PixiJS components used in timeline
extend({ Container, Graphics, Mesh, Text, MeshGeometry, MeshMaterial, Viewport })

// TypeScript declarations for custom JSX elements
declare module '@pixi/react' {
  interface PixiElements {
    viewport: import('@pixi/react').PixiReactElementProps<typeof Viewport>
  }
}
```

### Reusable Context Menu Component

```typescript
// Pattern: Portal-based context menu positioned at cursor
function ContextMenu({ items, position, onClose }: ContextMenuProps) {
  useEffect(() => {
    const handleClick = () => onClose()
    document.addEventListener('click', handleClick)
    return () => document.removeEventListener('click', handleClick)
  }, [onClose])

  return createPortal(
    <div
      className="absolute z-50 min-w-[160px] rounded-md border border-[#3a3a5a] bg-[#252542] py-1 shadow-lg"
      style={{ left: position.x, top: position.y }}
    >
      {items.map(item => (
        <button
          key={item.label}
          className="w-full px-4 py-2 text-left text-[13px] text-[#eeeeee] hover:bg-[#2a2a4a]"
          onClick={item.action}
        >
          {item.label}
        </button>
      ))}
    </div>,
    document.body
  )
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| @pixi/react v7 wrapper components | @pixi/react v8 `extend()` + JSX proxies | March 2025 | Complete rewrite for React 19. No more `<Container>` import from @pixi/react -- use `<pixiContainer>` after `extend()` |
| Zustand v4 implicit shallow comparison | Zustand v5 requires explicit `useShallow` | Oct 2024 | Multi-value selectors without `useShallow` cause infinite loops in v5 |
| Tailwind CSS v3 PostCSS + tailwind.config.js | Tailwind CSS v4 Vite plugin, CSS-only config | Jan 2025 | No config file needed. `@import "tailwindcss"` replaces `@tailwind` directives |
| pixi-viewport v5 for PixiJS 7 | pixi-viewport v6 for PixiJS 8 | 2024 | Updated types and API for PixiJS 8 compatibility |

**Deprecated/outdated:**
- `@pixi/react` v7 components (`<Container>`, `<Graphics>`, `<Sprite>`): Replaced by `pixi`-prefixed JSX elements in v8
- Zustand `create()` with custom equality function: Use `createWithEqualityFn` in v5, or use `useShallow`
- `tailwind.config.js` / `postcss.config.js`: Not needed with Tailwind v4 Vite plugin

## Open Questions

1. **C++ Engine Track/Clip Commands**
   - What we know: `dispatchCommand` works for instruments and effects. Command name mapping exists in bridge.cpp.
   - What's unclear: Whether clip.create, clip.move, clip.resize, track.add, etc. commands exist in the C++ engine yet, or if they need to be added.
   - Recommendation: If C++ commands don't exist yet, the timeline Zustand store can operate in "UI-only" mode first, with command dispatch added when engine commands are ready. Use a mock/stub layer for engine integration.

2. **Pre-computed Peak Data Pipeline**
   - What we know: The UI-SPEC defines `peakData?: Float32Array` on the Clip interface.
   - What's unclear: Whether the C++ engine has a peak computation API, or if this needs to be built.
   - Recommendation: For Phase 6, use synthesized/mock peak data for visual testing. Real peak computation can be a separate task or deferred to when audio import is implemented.

3. **pixi-viewport Event Scoping**
   - What we know: pixi-viewport attaches to the PIXI application's event system.
   - What's unclear: Exact configuration needed to prevent pixi-viewport from capturing keyboard shortcuts (Ctrl+Z for undo, Space for play) meant for the application.
   - Recommendation: Configure pixi-viewport with the canvas element as the event target. Use `stopPropagation` judiciously. Test keyboard shortcuts during implementation.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 3.2.4 |
| Config file | vitest.config.ts (root) |
| Quick run command | `pnpm test` |
| Full suite command | `pnpm test:all` |

### Phase Requirements to Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TL-01 | Timeline renders with tracks, scroll, zoom, playhead | unit + integration | `pnpm vitest run test/timeline-store.test.ts` | No - Wave 0 |
| TL-02 | MIDI clip CRUD operations | unit | `pnpm vitest run test/clip-operations.test.ts` | No - Wave 0 |
| TL-03 | Audio clip waveform rendering | unit | `pnpm vitest run test/waveform-utils.test.ts` | No - Wave 0 |
| TL-04 | Track add/remove/reorder/rename/color | unit | `pnpm vitest run test/timeline-store.test.ts` | No - Wave 0 |
| TL-05 | Snap-to-grid with configurable resolution | unit | `pnpm vitest run test/beat-math.test.ts` | No - Wave 0 |
| TL-06 | Loop region selection | unit | `pnpm vitest run test/timeline-store.test.ts` | No - Wave 0 |

### Sampling Rate
- **Per task commit:** `pnpm vitest run test/timeline-store.test.ts test/beat-math.test.ts`
- **Per wave merge:** `pnpm test:all`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `test/timeline-store.test.ts` -- covers TL-01, TL-02, TL-04, TL-06 (Zustand store actions and state transitions)
- [ ] `test/beat-math.test.ts` -- covers TL-05 (snap-to-grid, beat/pixel conversion)
- [ ] `test/clip-operations.test.ts` -- covers TL-02 (clip CRUD dispatch)
- [ ] `test/waveform-utils.test.ts` -- covers TL-03 (peak data to vertex conversion)
- [ ] Update vitest.config.ts to include `app/src/**/*.test.ts` if co-located tests are preferred
- [ ] Install `@testing-library/react` if component-level tests are needed (evaluate during planning)

## Project Constraints (from CLAUDE.md)

- **Audio Engine**: C++ with JUCE -- this phase does NOT modify C++ code, it builds the React UI layer
- **App Shell**: Electron with electron-vite -- add Tailwind and PixiJS to the existing renderer config
- **Canvas Rendering**: PixiJS 8 with @pixi/react -- CLAUDE.md specifies this, no alternatives
- **State Management**: Zustand -- CLAUDE.md specifies this, currently v5.0.12 installed
- **Styling**: Tailwind CSS 4 -- CLAUDE.md specifies this for non-canvas UI
- **Package Manager**: pnpm -- CLAUDE.md specifies this
- **Rejected Libraries**: Material UI, Ant Design, Redux, Konva.js, Three.js, Reactronica -- all explicitly rejected in CLAUDE.md
- **IPC Pattern**: contextBridge with strict contextIsolation -- established in Phase 1
- **Command Flow**: All mutations via dispatchCommand for undo/redo -- established in Phase 3

## Sources

### Primary (HIGH confidence)
- [PixiJS React v8 README](https://github.com/pixijs/pixi-react/blob/main/README.md) -- extend API, Application component, JSX patterns
- [PixiJS 8 Performance Tips](https://pixijs.com/8.x/guides/concepts/performance-tips) -- culling, Graphics optimization, draw call batching
- [PixiJS 8 Graphics Guide](https://pixijs.com/8.x/guides/components/scene-objects/graphics) -- Graphics API, Mesh for lines
- [Zustand v5 Migration Guide](https://github.com/pmndrs/zustand/blob/main/docs/migrations/migrating-to-v5.md) -- useShallow requirement, breaking changes
- [Tailwind CSS v4 Vite Installation](https://tailwindcss.com/docs) -- @tailwindcss/vite plugin setup
- [pixi-viewport GitHub](https://github.com/pixijs-userland/pixi-viewport) -- PixiJS 8 compatible, v6.0.3

### Secondary (MEDIUM confidence)
- [wavesurfer.js FAQ](https://wavesurfer.xyz/faq/) -- Pre-computed peaks support via load(url, peaks, duration)
- [PixiJS v8 Culling API](https://www.richardfu.net/optimizing-rendering-with-pixijs-v8-a-deep-dive-into-the-new-culling-api/) -- cullable property, cullArea, cullableChildren
- [PixiJS Discussion #7399](https://github.com/pixijs/pixijs/discussions/7399) -- Mesh line mode for high-performance line rendering

### Tertiary (LOW confidence)
- wavesurfer.js + PixiJS integration: No documented pattern exists for embedding wavesurfer.js inside a PixiJS stage. Recommendation to use Mesh lines is based on PixiJS performance documentation, not a verified DAW implementation.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All libraries verified via npm registry, compatibility confirmed (PixiJS 8 + @pixi/react 8 + React 19, pixi-viewport 6 + PixiJS 8, Tailwind 4 + Vite)
- Architecture: HIGH -- Patterns drawn from @pixi/react official docs, Zustand v5 migration guide, established Electron/IPC patterns from Phases 1-5
- Pitfalls: HIGH -- Zustand v5 useShallow requirement verified from migration guide, PixiJS Graphics vs Mesh performance from official docs, Tailwind v4 setup from official install guide

**Research date:** 2026-03-28
**Valid until:** 2026-04-28 (stable libraries, 30-day validity)
