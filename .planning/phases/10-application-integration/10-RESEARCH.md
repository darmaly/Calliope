# Phase 10: Application Integration - Research

**Researched:** 2026-03-29
**Domain:** React UI integration, transport controls, keyboard shortcuts, PixiJS waveform rendering
**Confidence:** HIGH

## Summary

Phase 10 integrates all existing panels (timeline, piano roll, mixer) into a unified DAW workspace. The codebase is mature with well-established patterns from Phases 6-8: Zustand stores with selectors, PixiJS via `@pixi/react` for canvas rendering, Tailwind CSS v4 for chrome UI, pointer events for interaction, and a capture-phase keydown pattern for panel-scoped shortcuts. The existing `App.tsx` already assembles timeline, piano roll, and mixer with `SplitDivider` resize -- this phase adds a transport bar above the timeline, consolidates keyboard shortcuts into a panel-focus-aware system, enhances audio clip waveform rendering, and adds window title integration.

All required libraries are already installed (React 19, Zustand 5, PixiJS 8, `@pixi/react`, `lucide-react`, Tailwind CSS v4). No new dependencies are needed. The IPC bridge already exposes transport play/stop/pause, BPM control, metronome toggle, undo/redo, and project save -- all required for transport bar and keyboard shortcuts.

**Primary recommendation:** Build the transport bar as a new React component, create a lightweight `app-store.ts` Zustand store for panel focus state, refactor the keyboard shortcut system to be focus-aware, and ensure audio clip waveform rendering works end-to-end with the existing `peaksToVertices` utility.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Panel arrangement is a vertical stack: timeline top, piano roll/mixer bottom -- refine and polish the existing layout, do not redesign
- Panel switching via click-to-toggle buttons in toolbar (unify existing Phase 7-8 toggle patterns into consistent panel bar)
- Drag-to-resize dividers between all panels using existing SplitDivider pattern from Phase 6
- Instrument/effect panels integrated into mixer channel strips (Phase 8) -- no separate panels needed
- Transport bar is a fixed top bar above the timeline -- always visible, standard DAW position
- Transport controls: Play/Stop/Record buttons, BPM display, time signature, playhead position in both MM:SS.ms and bars:beats format
- Audio clip waveforms render via PixiJS/WebGL on the timeline -- GPU-accelerated per success criteria
- Playhead synced via CSS overlay at ~60fps using requestAnimationFrame (Phase 6 established pattern)
- Standard DAW shortcut conventions: Space=play/stop, R=record, Ctrl+Z/Y=undo/redo, Ctrl+S=save, Delete=remove selected
- Active panel gets shortcut priority (piano roll > timeline > global), using capture-phase listener pattern from Phase 7
- Click-to-focus panels with subtle border highlight as visual focus indicator

### Claude's Discretion
- Transport bar exact pixel dimensions and styling
- BPM display edit interaction (click to type vs scroll)
- Waveform peak data generation and caching strategy
- Panel minimum/maximum sizes for resize constraints
- Focus border color and animation

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| UI-02 | Main layout with timeline, piano roll, mixer, and instrument/effect panels | Existing `App.tsx` already assembles all panels. Add `TransportBar` above timeline, create `app-store.ts` for focus tracking, wrap panels in focus-aware containers. Instrument/effect panels are in mixer channel strips (Phase 8). |
| UI-03 | Waveform rendering for audio clips using PixiJS/WebGL | `AudioClip.tsx` already renders waveforms via PixiJS `poly()` with `peaksToVertices()`. Already using `cullable={true}`. Enhancement: ensure peak data pipeline from engine is connected. |
| UI-04 | Responsive transport bar with BPM, time signature, play/stop/record controls | New `TransportBar` component. All IPC methods exist: `transportPlay`, `transportStop`, `transportPause`, `setBpm`, `setTimeSignature`. Position data from `usePlayhead` hook (rAF at ~30Hz). |
| UI-05 | Keyboard shortcuts for transport, undo/redo, save, and common operations | `useKeyboardShortcuts` and `usePianoRollShortcuts` already exist. Refactor into focus-aware system. Add Ctrl+S (save), R (record), L (loop). |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| React | 19.x | UI framework | Already installed, project standard |
| Zustand | 5.x | State management | Already installed, selector-based subscriptions prevent re-renders |
| PixiJS | 8.x | WebGL waveform rendering | Already installed, used for timeline canvas since Phase 6 |
| @pixi/react | 8.x | React-PixiJS bindings | Already installed, declarative PixiJS components |
| lucide-react | 1.7.x | Icons (Play, Square, Repeat, Timer, Piano, SlidersHorizontal) | Already installed, used throughout |
| Tailwind CSS | 4.x | Non-canvas UI styling | Already installed via @tailwindcss/vite plugin |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| zustand/shallow | 5.x | useShallow for multi-field selectors | When subscribing to multiple store fields to prevent unnecessary re-renders |

### Alternatives Considered
None needed -- all libraries are already in place from prior phases. No new dependencies required.

**Installation:**
```bash
# No new packages needed -- all dependencies already installed
```

## Architecture Patterns

### Recommended Project Structure
```
app/src/renderer/
  components/
    transport/
      TransportBar.tsx          # Fixed top bar container
      TransportControls.tsx     # Play/Stop/Record button group
      BpmDisplay.tsx            # Editable BPM readout
      TimeSignatureDisplay.tsx  # Time sig display (read-only)
      PositionDisplay.tsx       # MM:SS.ms + bars:beats
      LoopToggle.tsx            # Loop on/off
      MetronomeToggle.tsx       # Click track on/off
    shared/
      SplitDivider.tsx          # Move from piano-roll/ to shared/
  stores/
    app-store.ts                # Panel focus state
  hooks/
    use-keyboard-shortcuts.ts   # Refactored: focus-aware global shortcuts
    use-piano-roll-shortcuts.ts # Existing (minor refactor for focus system)
  utils/
    time-format.ts              # Beat-to-time and beat-to-bars conversion
```

### Pattern 1: Panel Focus Management via Zustand Store
**What:** A minimal Zustand store (`app-store.ts`) tracks which panel is focused. Panel containers attach `onPointerDown` to set focus. The keyboard shortcut handler reads `focusedPanel` to route shortcuts.
**When to use:** Whenever keyboard shortcuts need to be panel-contextual.
**Example:**
```typescript
// app-store.ts
import { create } from 'zustand'

type FocusablePanel = 'timeline' | 'piano-roll' | 'mixer'

interface AppState {
  focusedPanel: FocusablePanel
  setFocusedPanel: (panel: FocusablePanel) => void
}

export const useAppStore = create<AppState>((set) => ({
  focusedPanel: 'timeline',
  setFocusedPanel: (panel) => set({ focusedPanel: panel }),
}))
```

### Pattern 2: Transport Position Formatting
**What:** Convert beat position + BPM + time signature into human-readable MM:SS.ms and bars:beats formats. Pure utility functions, no state.
**When to use:** For the `PositionDisplay` component that updates at ~30Hz.
**Example:**
```typescript
// utils/time-format.ts
export function beatToTimeMs(beat: number, bpm: number): number {
  return (beat / bpm) * 60000
}

export function formatTimeMs(ms: number): string {
  const minutes = Math.floor(ms / 60000)
  const seconds = Math.floor((ms % 60000) / 1000)
  const millis = Math.floor(ms % 1000)
  return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}.${String(millis).padStart(3, '0')}`
}

export function beatToBarsBeats(
  beat: number,
  beatsPerBar: number
): { bar: number; beatInBar: number } {
  const bar = Math.floor(beat / beatsPerBar) + 1
  const beatInBar = Math.floor(beat % beatsPerBar) + 1
  return { bar, beatInBar }
}

export function formatBarsBeats(bar: number, beat: number): string {
  return `${String(bar).padStart(3, '0')}:${String(beat).padStart(2, '0')}`
}
```

### Pattern 3: Focus-Aware Keyboard Shortcut Routing
**What:** Refactor the existing two-hook shortcut system into a single document-level handler that checks `focusedPanel` from `app-store`. Global shortcuts (Space, R, Ctrl+Z, Ctrl+S) always fire. Panel-specific shortcuts only fire when their panel is focused.
**When to use:** Replaces the current `useKeyboardShortcuts` + `usePianoRollShortcuts` dual-hook approach.
**Example:**
```typescript
// Pseudostructure -- single capture-phase listener
document.addEventListener('keydown', (e) => {
  // 1. Skip form elements (INPUT, TEXTAREA, contentEditable)
  // 2. Global shortcuts (Space, R, L, Ctrl+Z, Ctrl+Shift+Z, Ctrl+S)
  // 3. Read focusedPanel from app-store
  // 4. Route to panel-specific handler based on focusedPanel
}, true) // capture phase for priority
```

### Pattern 4: Transport Button Component Pattern
**What:** Small stateless button components with lucide icons, using the project's established Tailwind pattern for interactive states.
**When to use:** For all transport bar buttons (Play, Stop, Record, Loop, Metronome).
**Example:**
```typescript
// Consistent pattern for toggle buttons in transport bar
<button
  onClick={handleClick}
  className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors ${
    active
      ? 'bg-[#6c63ff] text-white'
      : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee] hover:bg-[#4a4a6a]'
  }`}
  title={tooltip}
>
  <Icon size={16} />
</button>
```

### Anti-Patterns to Avoid
- **Multiple independent keydown listeners competing for events:** Consolidate into one handler with focus routing instead of adding more listeners per panel.
- **Re-rendering the entire transport bar on every beat position update:** Use Zustand selectors to subscribe only to `currentBeat` in `PositionDisplay`, not in the parent `TransportBar`.
- **Storing derived state (formatted time strings) in the store:** Compute `MM:SS.ms` and `bars:beats` in the component from raw `currentBeat` + `bpm` + `timeSignature`. These are pure derivations.
- **Moving panel toggles without updating the existing patterns:** The `TimelineToolbar` currently has Piano Roll and Mixer toggle buttons. Move them to `TransportBar` and remove from `TimelineToolbar` -- do not duplicate.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Icon rendering | SVG icons from scratch | `lucide-react` (Play, Square, Repeat, Timer, etc.) | Already in deps, consistent with existing UI |
| Waveform polygon math | New rendering approach | Existing `peaksToVertices()` utility | Already tested, uses the closed-polygon approach decided in Phase 6 |
| Transport IPC | New IPC channels | Existing `window.calliope.transport*` methods | All transport IPC was built in Phase 2 and exposed in preload |
| Playhead animation | New animation system | Existing `usePlayhead` hook (rAF at ~30Hz) | Already polls engine state, writes to `useTimelineStore` |
| Split divider | New resize component | Existing `SplitDivider` component | Battle-tested pointer event pattern from Phase 6 |

**Key insight:** This phase is primarily an integration and UI polish phase. Nearly all underlying functionality (transport IPC, waveform rendering, keyboard shortcuts, playhead sync) already exists. The work is composing these into a unified, polished layout.

## Common Pitfalls

### Pitfall 1: Keyboard Shortcut Conflicts Between Panels
**What goes wrong:** Piano roll Delete handler fires when timeline is focused, or vice versa.
**Why it happens:** Multiple `document.addEventListener('keydown', ...)` handlers compete without coordination.
**How to avoid:** Single capture-phase listener reads `focusedPanel` from `app-store` and routes to the correct handler. Remove separate `useKeyboardShortcuts` and `usePianoRollShortcuts` hooks.
**Warning signs:** Pressing Delete removes notes AND clips simultaneously; shortcuts work inconsistently depending on click history.

### Pitfall 2: Transport Position Display Jitter
**What goes wrong:** BPM/time numerals visually shift left/right as digits change width (e.g., "1" is narrower than "0").
**Why it happens:** Default proportional font rendering.
**How to avoid:** Apply `font-variant-numeric: tabular-nums` (Tailwind: `tabular-nums` or `[font-variant-numeric:tabular-nums]`) to all numeric displays in the transport bar.
**Warning signs:** Numbers appear to "jitter" or shift horizontally during playback.

### Pitfall 3: BPM Edit Input Losing Focus
**What goes wrong:** Click on BPM to edit, start typing, then an rAF update re-renders the component and blurs the input.
**Why it happens:** The BPM display component subscribes to `bpm` from the store, and the playhead poll continuously writes transport state including `bpm`.
**How to avoid:** Track an `isEditing` local state. When `isEditing` is true, render the input and do NOT subscribe to the store's `bpm` value -- use a local `editValue` state instead. Only commit the new BPM on Enter/blur.
**Warning signs:** Input loses focus immediately after clicking, or typed digits get overwritten.

### Pitfall 4: Panel Focus Stealing from Input Elements
**What goes wrong:** Clicking a BPM input or any text field inside a panel changes panel focus, and the keyboard shortcut handler starts intercepting keystrokes meant for the input.
**Why it happens:** The `pointerdown` focus handler fires before the input gets focus.
**How to avoid:** The shortcut handler already checks `e.target.tagName === 'INPUT'` -- keep this guard. Panel focus via `pointerdown` is fine because the input check in the keydown handler prevents shortcut interception.
**Warning signs:** Typing in BPM input triggers play/stop (Space), or other shortcuts fire while editing.

### Pitfall 5: SplitDivider Import Path Breakage
**What goes wrong:** Moving `SplitDivider` from `components/piano-roll/` to `components/shared/` breaks existing imports.
**Why it happens:** `App.tsx` and `PianoRollPanel.tsx` both import from the old path.
**How to avoid:** Update all import paths when moving the file. Search for `'../piano-roll/SplitDivider'` and `'./piano-roll/SplitDivider'` across the codebase.
**Warning signs:** Build errors after file move.

### Pitfall 6: Window Title Not Updating on Save/Load
**What goes wrong:** Window title shows stale project name after save-as or load.
**Why it happens:** `document.title` set once but not subscribed to store changes.
**How to avoid:** Use a `useEffect` that subscribes to `projectName` and `isDirty` from `useProjectStore` and updates `document.title` reactively.
**Warning signs:** Title says "Untitled" after loading a project file.

## Code Examples

### Beat-to-Time Conversion (for PositionDisplay)
```typescript
// Verified pattern: standard DAW time computation
export function beatToTimeMs(beat: number, bpm: number): number {
  // beats / (beats/minute) = minutes, * 60000 = milliseconds
  return (beat / bpm) * 60000
}
```

### Existing IPC Methods Available for Transport Bar
```typescript
// From app/src/preload/index.ts -- already exposed
window.calliope.transportPlay()      // Play
window.calliope.transportStop()      // Stop
window.calliope.transportPause()     // Pause
window.calliope.setBpm(bpm)          // Set BPM
window.calliope.setTimeSignature(num, den) // Set time sig
window.calliope.setLoopRegion(start, end, enabled) // Loop
window.calliope.setMetronomeEnabled(enabled) // Metronome
window.calliope.commandUndo()        // Undo
window.calliope.commandRedo()        // Redo
window.calliope.projectSave()        // Save
```

### Existing Transport State Shape (from engine)
```typescript
// From app/src/renderer/types/calliope.d.ts
interface TransportState {
  state: 'stopped' | 'playing' | 'paused'
  bpm: number
  timeSigNumerator: number
  timeSigDenominator: number
  samplePosition: number
  ppqPosition: number       // <-- beat position used for display
  looping: boolean
  loopStartBeat: number
  loopEndBeat: number
}
```

### Existing Playhead Sync Pattern (reuse for transport bar)
```typescript
// From app/src/renderer/hooks/use-playhead.ts
// Already polls engine at ~30Hz via rAF, writes to useTimelineStore.setTransportState()
// TransportBar components subscribe to useTimelineStore for currentBeat, bpm, etc.
```

### Existing Waveform Rendering (already functional)
```typescript
// From app/src/renderer/components/timeline/AudioClip.tsx
// Uses peaksToVertices() from utils/waveform.ts
// Renders via <pixiGraphics draw={drawWaveform} />
// Fallback: dashed center line when no peak data
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Separate shortcut hooks per panel | Single focus-aware shortcut handler | This phase | Eliminates shortcut conflicts, cleaner architecture |
| No transport bar (controls scattered) | Dedicated TransportBar component | This phase | Standard DAW UX, always-visible temporal context |
| Panel toggle buttons in TimelineToolbar | Panel toggles in TransportBar | This phase | Unified navigation in one bar |

## Open Questions

1. **Record button behavior without audio recording support**
   - What we know: Audio recording from microphone is out of scope. The preload bridge has no `transportRecord()` method.
   - What's unclear: Should the Record button be included but disabled, or omitted entirely?
   - Recommendation: Include it as a visual element with a disabled/placeholder state. The UI spec calls for it. It can be wired up in a future phase. For now, clicking it can toggle a visual "armed" state without triggering anything in the engine.

2. **Peak data pipeline from C++ engine**
   - What we know: `AudioClip.tsx` renders waveforms from `clip.peakData` (Float32Array). The `peaksToVertices()` utility works. Phase 6 decisions note "engine clip dispatch wrapped in try/catch (C++ commands not yet implemented, store-only for now)."
   - What's unclear: Whether the C++ engine currently generates and sends peak data for loaded audio files.
   - Recommendation: For UI-03 compliance, ensure the existing PixiJS waveform rendering works with mock/synthetic peak data. The `generateMockPeaks()` utility exists for this purpose. The actual engine integration is an audio pipeline concern, not a UI concern.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 3.x |
| Config file | `vitest.config.ts` (project root) |
| Quick run command | `pnpm test` |
| Full suite command | `pnpm test:all` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| UI-02 | App layout renders transport bar, timeline, piano roll, mixer | unit | `pnpm vitest run test/app-layout.test.ts -x` | No -- Wave 0 |
| UI-03 | Waveform rendering produces correct polygon vertices | unit | `pnpm vitest run test/waveform-utils.test.ts -x` | Yes (existing) |
| UI-04 | Beat-to-time and beat-to-bars formatting | unit | `pnpm vitest run test/time-format.test.ts -x` | No -- Wave 0 |
| UI-05 | Keyboard shortcuts dispatch correct actions by focus state | unit | `pnpm vitest run test/keyboard-shortcuts.test.ts -x` | No -- Wave 0 |

### Sampling Rate
- **Per task commit:** `pnpm test`
- **Per wave merge:** `pnpm test:all`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `test/time-format.test.ts` -- covers UI-04 (beat-to-time, beat-to-bars conversion functions)
- [ ] `test/keyboard-shortcuts.test.ts` -- covers UI-05 (shortcut routing logic, panel focus dispatch)
- [ ] `test/app-layout.test.ts` -- covers UI-02 (layout composition assertions -- may be minimal without React Testing Library)

Note: Existing `test/waveform-utils.test.ts` already covers UI-03 waveform vertex computation. React component rendering tests are limited without React Testing Library (not installed) -- focus unit tests on pure logic functions (time formatting, shortcut routing logic).

## Sources

### Primary (HIGH confidence)
- Project codebase: `app/src/renderer/` -- all existing components, stores, hooks, utilities examined directly
- `app/src/preload/index.ts` -- verified all IPC methods available for transport bar
- `app/src/renderer/types/calliope.d.ts` -- verified TransportState shape from engine
- `10-CONTEXT.md` -- locked decisions for layout, transport bar, shortcuts
- `10-UI-SPEC.md` -- detailed visual specifications for all new components

### Secondary (MEDIUM confidence)
- Zustand v5 patterns (verified against codebase usage, consistent with `create<>` and `useShallow` patterns)
- PixiJS 8 `@pixi/react` patterns (verified against existing `AudioClip.tsx`, `TimelineCanvas.tsx`)

### Tertiary (LOW confidence)
- None -- all findings based on direct codebase examination

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries already installed and used extensively
- Architecture: HIGH -- patterns directly observed in existing codebase, no new paradigms
- Pitfalls: HIGH -- pitfalls derived from code inspection of actual existing patterns and their interaction points

**Research date:** 2026-03-29
**Valid until:** 2026-04-28 (stable -- no dependency changes expected)
