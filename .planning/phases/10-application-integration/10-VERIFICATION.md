---
phase: 10-application-integration
verified: 2026-03-29T21:30:00Z
status: gaps_found
score: 10/11 must-haves verified
gaps:
  - truth: "Panel toggle buttons (Piano Roll, Mixer) appear in the transport bar right section"
    status: partial
    reason: "Piano Roll toggle in TransportBar wires correctly to piano-roll-store. MixerToggle renders a button with no onPointerDown handler and no connection to mixer-store.toggleMixerVisible(). Clicking Mixer in the transport bar does nothing. The mixer-store exists and has toggleMixerVisible() but TransportBar.MixerToggle does not call it."
    artifacts:
      - path: "app/src/renderer/components/transport/TransportBar.tsx"
        issue: "MixerToggle function (lines 86-98) renders a button with no handler. Mixer toggle is not wired to useMixerStore."
    missing:
      - "Import useMixerStore in TransportBar.tsx"
      - "Add mixerVisible = useMixerStore((s) => s.mixerVisible) selector to MixerToggle"
      - "Add onPointerDown handler to MixerToggle button that calls useMixerStore.getState().toggleMixerVisible()"
---

# Phase 10: Application Integration Verification Report

**Phase Goal:** All panels unite into a cohesive DAW workspace with professional navigation, rendering, and shortcuts
**Verified:** 2026-03-29T21:30:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Transport bar displays above the timeline with play/stop/record buttons, BPM, time signature, and position | VERIFIED | TransportBar.tsx renders in App.tsx line 82 as first child. TransportControls, BpmDisplay, TimeSignatureDisplay, PositionDisplay all present and wired to useTimelineStore. |
| 2 | BPM value is editable via click-to-type input with Enter to confirm and Escape to cancel | VERIFIED | BpmDisplay.tsx implements isEditing local state, editValue, startEdit/commit/cancel pattern with Enter/Escape/blur handlers. Calls window.calliope.setBpm(val) on commit. |
| 3 | Position display shows both MM:SS.ms and bars:beats format with tabular-nums to prevent jitter | VERIFIED | PositionDisplay.tsx renders both formats using beatToTimeMs/formatTimeMs and beatToBarsBeats/formatBarsBeats. fontVariantNumeric: 'tabular-nums' applied. Uses useShallow selector. |
| 4 | Loop and metronome toggles show active/inactive state | VERIFIED | LoopToggle.tsx and MetronomeToggle.tsx use local state with accent/inactive color classes. Both call optional engine IPC (setLoopRegion?./setMetronomeEnabled?.). |
| 5 | Panel toggle buttons (Piano Roll, Mixer) appear in the transport bar right section | PARTIAL | PianoRollToggle wires to piano-roll-store and functions. MixerToggle is a non-functional stub — renders button with no handler, not connected to mixer-store.toggleMixerVisible(). |
| 6 | SplitDivider is available as a shared component | VERIFIED | app/src/renderer/components/shared/SplitDivider.tsx exists (49 lines). Old piano-roll/SplitDivider.tsx is a one-line re-export. App.tsx imports from shared path. |
| 7 | Keyboard shortcuts for transport (Space, R, L), undo/redo (Ctrl+Z/Shift+Z), and save (Ctrl+S) work globally | VERIFIED | routeShortcut in use-keyboard-shortcuts.ts maps all global shortcuts. 27 tests pass confirming routing logic. Unified handler dispatches to window.calliope and useProjectStore.getState().save(). |
| 8 | Panel-specific shortcuts (Delete, Ctrl+A, Ctrl+D, Escape, Q) route to the focused panel only | VERIFIED | routeShortcut branches on focusedPanel parameter. Tests confirm Delete routes to timeline:delete-selected on timeline focus and piano-roll:delete-selected on piano-roll focus. Q only fires on piano-roll. |
| 9 | Clicking a panel visually focuses it with a 2px left accent border | VERIFIED | App.tsx focusBorder() helper returns 'border-l-2 border-[#6c63ff]' for focused panel. onPointerDown set on all three panel containers (lines 85, 95, 107). |
| 10 | Audio clip waveforms render via PixiJS with peak data or fallback dashed line | VERIFIED | AudioClip.tsx uses peaksToVertices() for real peak data (poly fill) and draws dashed center line when clip.peakData is absent. cullable={true} set. |
| 11 | Window title shows project name and dirty indicator | VERIFIED | App.tsx useEffect at line 34: document.title = `${isDirty ? '* ' : ''}${projectName} - LuneyTunes`. Reactive to projectName and isDirty from useProjectStore. |

**Score:** 10/11 truths verified (1 partial)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `app/src/renderer/stores/app-store.ts` | Panel focus state management | VERIFIED | Exports useAppStore with focusedPanel ('timeline' default) and setFocusedPanel action |
| `app/src/renderer/utils/time-format.ts` | Beat-to-time and beat-to-bars conversion | VERIFIED | Exports beatToTimeMs, formatTimeMs, beatToBarsBeats, formatBarsBeats — all 4 functions |
| `app/src/renderer/components/transport/TransportBar.tsx` | Top-level transport bar container | VERIFIED | Exports TransportBar, imports all sub-components, renders layout with separators and panel toggles |
| `app/src/renderer/components/transport/TransportControls.tsx` | Play/Stop/Record button group | VERIFIED | Exports TransportControls, uses isPlaying from useTimelineStore, calls transportPlay/transportStop |
| `app/src/renderer/components/transport/BpmDisplay.tsx` | Editable BPM readout | VERIFIED | Full click-to-edit implementation with local isEditing/editValue state, Enter/Escape/blur |
| `app/src/renderer/components/transport/PositionDisplay.tsx` | Dual-format position display | VERIFIED | Renders MM:SS.ms and bars:beats with useShallow selector, tabular-nums |
| `app/src/renderer/components/transport/TimeSignatureDisplay.tsx` | Read-only time signature | VERIFIED | Exports TimeSignatureDisplay, reads from useTimelineStore |
| `app/src/renderer/components/transport/LoopToggle.tsx` | Loop toggle button | VERIFIED | Active/inactive state, Repeat icon, calls optional setLoopRegion?. |
| `app/src/renderer/components/transport/MetronomeToggle.tsx` | Metronome toggle button | VERIFIED | Active/inactive state, Timer icon, calls optional setMetronomeEnabled?. |
| `app/src/renderer/components/shared/SplitDivider.tsx` | Shared drag divider component | VERIFIED | 49 lines, substantive implementation |
| `test/time-format.test.ts` | Unit tests for time formatting | VERIFIED | 12 tests, all pass |
| `app/src/renderer/hooks/use-keyboard-shortcuts.ts` | Focus-aware unified keyboard shortcut handler | VERIFIED | Exports routeShortcut pure function and useKeyboardShortcuts hook |
| `test/keyboard-shortcuts.test.ts` | Unit tests for shortcut routing logic | VERIFIED | 27 tests, all pass |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| TransportBar.tsx | useTimelineStore | useTimelineStore selector | WIRED | BpmDisplay, PositionDisplay, TimeSignatureDisplay, TransportControls all subscribe to timeline-store |
| TransportControls.tsx | window.calliope | transportPlay/transportStop IPC calls | WIRED | Lines 10-13 dispatch to window.calliope.transportPlay() and transportStop() |
| use-keyboard-shortcuts.ts | app-store.ts | useAppStore.getState().focusedPanel | WIRED | Line 108 reads focusedPanel for routing decisions |
| App.tsx | app-store.ts | setFocusedPanel on pointerdown | WIRED | Lines 85, 95, 107 — all three panel containers set focused panel on pointerdown |
| App.tsx | project-store.ts | document.title from projectName + isDirty | WIRED | useEffect line 34 updates document.title reactively |
| TransportBar.tsx (MixerToggle) | mixer-store.ts | toggleMixerVisible | NOT WIRED | MixerToggle has no handler, not connected to useMixerStore |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|-------------------|--------|
| PositionDisplay.tsx | currentBeat, bpm, beatsPerBar | useTimelineStore (Zustand) | Yes — updated by usePlayhead hook from engine events | FLOWING |
| BpmDisplay.tsx | bpm | useTimelineStore | Yes — set from engine transport state | FLOWING |
| AudioClip.tsx | clip.peakData | Clip object from timeline-store | Conditional — Float32Array when present, fallback when absent | FLOWING (with fallback) |
| TransportBar.MixerToggle | (none connected) | N/A | N/A — no state reads | DISCONNECTED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| time-format tests pass | `pnpm vitest run test/time-format.test.ts` | 12/12 passing | PASS |
| keyboard-shortcuts tests pass | `pnpm vitest run test/keyboard-shortcuts.test.ts` | 27/27 passing | PASS |
| TypeScript compiles cleanly | `pnpm exec tsc --noEmit -p app/tsconfig.json` | No errors | PASS |
| routeShortcut exports from hook file | grep export routeShortcut | Found at line 29 of use-keyboard-shortcuts.ts | PASS |
| usePianoRollShortcuts no longer imported by PianoRollPanel | grep usePianoRollShortcuts in PianoRollPanel.tsx | Not found | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|---------|
| UI-02 | 10-01, 10-02 | Main layout with timeline, piano roll, mixer, and instrument/effect panels | PARTIAL | Timeline, piano roll, and mixer panels are in the unified layout. TransportBar shows panel toggles. However, MixerToggle button in TransportBar is non-functional — clicking it does nothing. App.tsx correctly shows mixer via showMixer flag from mixer-store, but the TransportBar toggle doesn't trigger it. |
| UI-03 | 10-02 | Waveform rendering for audio clips using PixiJS/WebGL | SATISFIED | AudioClip.tsx renders waveforms via PixiJS Graphics.poly() with peaksToVertices() for peak data, and falls back to dashed center line. cullable={true} for WebGL culling. |
| UI-04 | 10-01 | Responsive transport bar with BPM, time signature, play/stop/record controls | SATISFIED | TransportBar with all 7 sub-components: TransportControls (play/stop/record), BpmDisplay (editable), TimeSignatureDisplay, PositionDisplay (dual format), LoopToggle, MetronomeToggle. Renders above timeline in App.tsx. |
| UI-05 | 10-02 | Keyboard shortcuts for transport, undo/redo, save, and common operations | SATISFIED | Focus-aware routeShortcut function covers all specified shortcuts. 27 tests pass. Global shortcuts (Space, R, L, Ctrl+Z, Ctrl+S) always fire. Panel-specific shortcuts route to timeline/piano-roll/mixer only when focused. Input guard prevents shortcuts when typing. |

**Note:** UI-03 was also claimed by plan 10-02. The AudioClip.tsx waveform rendering was already present from Phase 6 (TL-03) — plan 10-02 verified it rather than implementing it. No regression found.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| TransportBar.tsx | 86-98 | MixerToggle function — button with no onClick/onPointerDown handler | Blocker | Mixer panel cannot be opened via transport bar. User expectation (panel toggle in transport) not met. |
| TransportBar.tsx | 87-88 | Comment "Placeholder: mixer-store does not exist yet" — mixer-store DOES exist | Warning | Misleading comment — mixer-store.ts exists and has toggleMixerVisible(). The stub is unnecessary. |
| app/src/renderer/hooks/use-piano-roll-shortcuts.ts | 1-125 | Full hook implementation (dead code — no longer called) | Warning | Duplicate shortcut logic that could cause confusion. Not a functional blocker since the file is not imported by any active component. |

### Human Verification Required

#### 1. Transport bar visual layout

**Test:** Launch app with `pnpm run dev`. Inspect the transport bar at the top.
**Expected:** 48px height bar with play/pause/record, BPM (clickable), time signature, time+position displays, loop+metronome, Piano Roll/Mixer toggles on right.
**Why human:** Cannot verify visual appearance or exact pixel layout programmatically.

#### 2. BPM click-to-edit interaction

**Test:** Click the BPM value. Type a new value. Press Enter. Verify BPM changes. Open again, press Escape — verify it reverts.
**Expected:** Input appears immediately on click, commits on Enter/blur, cancels on Escape. No digit jitter during playback.
**Why human:** Cannot test click interaction or visual jitter from code alone.

#### 3. Mixer toggle does NOT work from transport bar

**Test:** Click "Mixer" button in transport bar right section.
**Expected (BUG):** Nothing happens. The mixer panel does not open.
**Why human:** Runtime behavior of the non-functional stub needs human confirmation of the gap.

#### 4. Panel focus visual

**Test:** Click on the timeline area, piano roll area, and mixer area in sequence.
**Expected:** A 2px purple left border appears on the active panel. Confirm it moves correctly.
**Why human:** Visual rendering of the focus border requires runtime check.

#### 5. Keyboard shortcut context-awareness

**Test:** With timeline focused, press Delete with a clip selected. Then open piano roll, click it to focus, press Delete with a note selected.
**Expected:** Delete deletes clips when timeline focused; deletes notes when piano roll focused.
**Why human:** Requires interactive UI session to verify focus state and keyboard event routing.

### Gaps Summary

**1 gap blocking goal achievement:**

**MixerToggle stub in TransportBar.tsx (lines 86-98)** — The `MixerToggle` component in the transport bar renders a decorative button with no event handler. The mixer-store already exists (created in an earlier phase) and exposes `toggleMixerVisible()`, but `MixerToggle` was written as a placeholder before the mixer-store was available. Now that the mixer-store is present, the stub was never updated. As a result, users cannot open the mixer panel from the transport bar, which is the primary UX affordance for the mixer toggle. App.tsx does correctly show/hide the mixer panel based on `useMixerStore`'s `mixerVisible` state — the only missing piece is wiring the TransportBar button to `toggleMixerVisible()`.

**Fix required:** In TransportBar.tsx, update `MixerToggle` to import `useMixerStore`, read `mixerVisible` for active state styling, and call `useMixerStore.getState().toggleMixerVisible()` on `onPointerDown`.

---

*Verified: 2026-03-29T21:30:00Z*
*Verifier: Claude (gsd-verifier)*
