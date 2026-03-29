---
phase: 08-mixer
verified: 2026-03-29T16:02:00Z
status: gaps_found
score: 6/8 must-haves verified
re_verification: false
gaps:
  - truth: "Effect slots allow add, remove, bypass, and click to open parameter popover"
    status: partial
    reason: "Add, remove, bypass, and parameter popover click are wired. Reorder (drag-and-drop) is unimplemented in UI. MIX-03 explicitly requires reorder per effect."
    artifacts:
      - path: "app/src/renderer/components/mixer/EffectSlotList.tsx"
        issue: "No onReorder prop, no draggable attribute, no HTML5 drag-and-drop wiring"
      - path: "app/src/renderer/components/mixer/EffectSlot.tsx"
        issue: "No drag handles, no onDragStart/onDrop handlers"
    missing:
      - "Add onReorder prop to EffectSlotList"
      - "Implement HTML5 drag-and-drop or pointer-based reorder in EffectSlotList"
      - "Wire reorder to useMixerStore.getState().reorderTrackEffect / reorderMasterEffect"

  - truth: "Mixer panel toggles from toolbar button, appears below timeline/piano-roll"
    status: partial
    reason: "Toggle open/close works. SplitDivider is present but handleMixerDividerDrag is an empty no-op and mixerHeight is hardcoded to 300. Panel is not resizable."
    artifacts:
      - path: "app/src/renderer/App.tsx"
        issue: "handleMixerDividerDrag callback is empty; mixerHeight = 300 hardcoded, not read from store"
    missing:
      - "Add mixerPanelHeight to mixer-store (or local state)"
      - "Implement handleMixerDividerDrag to compute and persist new height"
      - "Read mixerHeight from store/state instead of hardcoding 300"

human_verification:
  - test: "Open mixer panel and verify visual layout"
    expected: "Channel strips render for each track, Master strip is rightmost and wider, faders/knobs/buttons all visible"
    why_human: "Plan 03 Task 3 is a blocking human-verify checkpoint that was not completed before summary was written"
  - test: "Play audio and verify level meters animate"
    expected: "Green/amber/red gradient bars animate at ~30Hz when audio plays"
    why_human: "Requires audio playback; C++ engine must be running"
  - test: "Drag a volume fader and verify audio volume changes"
    expected: "Dragging fader changes track volume; dB readout updates; double-click resets to 0dB"
    why_human: "Requires interactive pointer events in running Electron app"
---

# Phase 8: Mixer Verification Report

**Phase Goal:** Users can mix tracks with volume, pan, mute/solo, effect inserts, and real-time level meters
**Verified:** 2026-03-29T16:02:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Each track displays a channel strip with volume fader, pan knob, mute, solo, arm buttons | VERIFIED | ChannelStrip.tsx: Fader + PanKnob + M/S/Arm buttons all wired to useMixerStore and useTimelineStore |
| 2 | Master strip is always visible, rightmost, wider, with its own effect chain | VERIFIED | MasterStrip.tsx: 88px width, `#2a2a4e` background, `#6c63ff` left border, EffectSlotList wired to masterEffects |
| 3 | Effect slots allow add, remove, bypass, and click to open parameter popover | PARTIAL | Add/remove/bypass/popover wired. Reorder not implemented in UI. |
| 4 | Level meters animate at ~30Hz with green/amber/red gradient and peak hold | VERIFIED | LevelMeter.tsx: Canvas 2D createLinearGradient with #22c55e/#f59e0b/#ef4444; useMeterPolling uses requestAnimationFrame polling window.calliope.getMeterLevels() |
| 5 | Mixer panel toggles from toolbar button, appears below timeline/piano-roll | PARTIAL | Toggle works (mixerVisible, toggleMixerVisible, SlidersHorizontal button). SplitDivider present but handleMixerDividerDrag is empty; height hardcoded to 300. |
| 6 | Fader range is -inf to +6dB with dB markings and double-click to reset | VERIFIED | Fader.tsx: faderToGain/gainToFader logarithmic mapping, formatDb readout, onPointerDown double-click reset to onChange(1.0) |
| 7 | Pan knob rotates via pointer drag, double-click resets to center | VERIFIED | PanKnob.tsx: setPointerCapture + vertical pointer drag, double-click resets to onChange(0), panToString label |
| 8 | C++ engine computes per-track and master RMS/peak in processBlock using atomics | VERIFIED | insert_chain_processor.cpp and master_bus.cpp: memory_order_relaxed stores at end of processBlock |

**Score:** 6/8 truths verified (2 partial)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `app/src/renderer/types/mixer.ts` | Mixer types | VERIFIED | EffectSlotInfo, LevelData, EFFECT_TYPES, EffectType |
| `app/src/renderer/stores/mixer-store.ts` | Zustand mixer store | VERIFIED | 153 lines; trackVolumes/trackPans/trackLevels/trackEffects/masterVolume/masterEffects; full CRUD actions |
| `app/src/renderer/utils/mixer-helpers.ts` | dB/linear/fader helpers | VERIFIED | dbToLinear, linearToDb, formatDb, faderToGain, gainToFader, panToString, clampVolume, clampPan |
| `app/src/renderer/components/mixer/Fader.tsx` | Vertical fader | VERIFIED | 182 lines; faderToGain, onPointerDown pointer capture, dB scale markings |
| `app/src/renderer/components/mixer/PanKnob.tsx` | Rotary pan | VERIFIED | 92 lines; setPointerCapture, panToString, double-click reset |
| `app/src/renderer/components/mixer/LevelMeter.tsx` | Canvas 2D meter | VERIFIED | 74 lines; useRef+useEffect, createLinearGradient, green/amber/red stops |
| `app/src/renderer/components/mixer/EffectSlot.tsx` | Effect slot | VERIFIED | 162 lines; add dropdown, bypass toggle, context menu for remove |
| `app/src/renderer/components/mixer/EffectSlotList.tsx` | Effect slot list | PARTIAL | 49 lines; renders filled slots + empty add slot; no reorder UI |
| `app/src/renderer/components/mixer/EffectParamPopover.tsx` | Effect parameter editor | VERIFIED | 145 lines; createPortal, per-effect-type param sliders, outside-click close |
| `app/src/renderer/components/mixer/ChannelStrip.tsx` | Per-track strip | VERIFIED | 202 lines; useMixerStore + useTimelineStore; Fader/PanKnob/LevelMeter/EffectSlotList/M/S/Arm |
| `app/src/renderer/components/mixer/MasterStrip.tsx` | Master strip | VERIFIED | 125 lines; #2a2a4e bg, 88px, master effects wired |
| `app/src/renderer/components/mixer/MixerPanel.tsx` | Mixer container | VERIFIED | useMeterPolling called, horizontal scroll of ChannelStrips, MasterStrip pinned, empty state |
| `app/src/renderer/components/mixer/MixerToolbar.tsx` | Mixer toolbar header | VERIFIED | 22 lines; "Mixer" label |
| `app/src/renderer/hooks/use-meter-polling.ts` | RAF meter poll hook | VERIFIED | requestAnimationFrame loop when mixerVisible, calls getMeterLevels, setTrackLevel/setMasterLevel |
| `engine/include/calliope/insert_chain_processor.h` | MeterData struct | VERIFIED | struct MeterData with atomic<float> rmsLeft/rmsRight/peakLeft/peakRight; getMeterData() accessor |
| `engine/src/insert_chain_processor.cpp` | Meter computation | VERIFIED | RMS + peak computed after chain_.process(), stored with memory_order_relaxed |
| `engine/include/calliope/master_bus.h` | Master MeterData | VERIFIED | Same MeterData struct and getMeterData() |
| `engine/src/master_bus.cpp` | Master meter computation | VERIFIED | RMS + peak computed after volume application, memory_order_relaxed |
| `engine/include/calliope/audio_graph.h` | AllMeterLevels | VERIFIED | struct AllMeterLevels with tracks vector + master; getMeterLevels() method |
| `engine/src/audio_graph.cpp` | getMeterLevels() impl | VERIFIED | Reads all 3 track chains + masterBus atomics into AllMeterLevels |
| `engine/include/calliope/engine.h` | getMeterLevels delegation | VERIFIED | AudioGraph::AllMeterLevels getMeterLevels() const |
| `native/src/bridge.cpp` | GetMeterLevels bridge | VERIFIED | ThreadSafeFunction + std::thread pattern, builds AllMeterLevels into Napi::Object |
| `native/src/addon.cpp` | getMeterLevels export | VERIFIED | exports.Set("getMeterLevels", ...) at line 50 |
| `app/src/main/index.ts` | IPC handler | VERIFIED | ipcMain.handle('engine:meter:getLevels') returns addon.getMeterLevels() |
| `app/src/preload/index.ts` | Preload API | VERIFIED | getMeterLevels, setTrackVolume, setTrackPan all present and invoke correct IPC channels |
| `app/src/renderer/types/calliope.d.ts` | Type declarations | VERIFIED | getMeterLevels, setTrackVolume, setTrackPan typed |
| `app/src/renderer/components/timeline/TimelineToolbar.tsx` | Mixer toggle button | VERIFIED | SlidersHorizontal icon + "Mixer" text; onClick calls toggleMixerVisible() |
| `app/src/renderer/App.tsx` | Mixer panel in layout | PARTIAL | MixerPanel rendered when showMixer; SplitDivider present but handleMixerDividerDrag is empty no-op; mixerHeight hardcoded |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `ChannelStrip.tsx` | `mixer-store.ts` | useMixerStore selectors | WIRED | trackVolumes, trackPans, trackLevels, trackEffects all read via individual selectors |
| `ChannelStrip.tsx` | `timeline-store.ts` | useTimelineStore for mute/solo/arm | WIRED | toggleMute, toggleSolo, toggleArm, selectTrack all called |
| `use-meter-polling.ts` | `mixer-store.ts` | setTrackLevel / setMasterLevel from getMeterLevels IPC | WIRED | Polling loop calls window.calliope.getMeterLevels() then setTrackLevel/setMasterLevel |
| `App.tsx` | `mixer-store.ts` | mixerVisible controls mixer rendering | WIRED | useMixerStore((s) => s.mixerVisible) → showMixer conditional |
| `TimelineToolbar.tsx` | `mixer-store.ts` | toggleMixerVisible on button click | WIRED | useMixerStore.getState().toggleMixerVisible() on onClick |
| `preload/index.ts` | `main/index.ts` | IPC invoke engine:meter:getLevels | WIRED | preload invokes 'engine:meter:getLevels'; main handles and calls addon.getMeterLevels() |
| `bridge.cpp` | `engine.h` | Engine::getInstance().getMeterLevels() | WIRED | GetMeterLevels calls engine.getMeterLevels() which delegates to AudioGraph |
| `insert_chain_processor.cpp` | `insert_chain_processor.h` | MeterData atomic writes in processBlock | WIRED | meterData_.rmsLeft.store/peakLeft.store with memory_order_relaxed after chain processing |
| `App.tsx` SplitDivider → mixer height | `mixer-store.ts` | handleMixerDividerDrag callback | NOT_WIRED | handleMixerDividerDrag is an empty function body; mixerHeight hardcoded to 300 |
| `EffectSlotList.tsx` → reorder | `mixer-store.ts` | reorderTrackEffect action | NOT_WIRED | No drag-and-drop UI; reorderTrackEffect/reorderMasterEffect actions exist in store but unreachable from UI |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `LevelMeter.tsx` | leftLevel, rightLevel, leftPeak, rightPeak | ChannelStrip reads trackLevels[track.id] from mixer-store | Yes — meter polling hook writes peakLeft/peakRight from C++ bridge on each RAF frame | FLOWING |
| `MasterStrip.tsx` | masterLevel | mixer-store.masterLevel | Yes — meter polling writes setMasterLevel from 'master' key in getMeterLevels response | FLOWING |
| `ChannelStrip.tsx` | volume (trackVolumes), pan (trackPans) | mixer-store.trackVolumes/trackPans | Yes — written by setTrackVolume/setTrackPan actions which also dispatch to C++ engine | FLOWING |
| `EffectSlotList.tsx` | slots (trackEffects) | mixer-store.trackEffects | Yes — addTrackEffect/removeTrackEffect/bypassTrackEffect wired; reorderTrackEffect unreachable | PARTIAL |

**Note:** LevelMeter receives `leftPeak` = `level?.peakL` which is the same value as `leftLevel`. The store stores only peak values (no separate RMS). The visual peak-hold indicator and the bar level are identical. This is a cosmetic limitation — meters will work but the peak hold line will coincide with the top of the bar during active signal.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Mixer tests pass | `npx vitest run test/mixer-store.test.ts test/mixer-helpers.test.ts` | 55 tests passed | PASS |
| Full test suite | `npx vitest run` | 170 passed, 5 todo, 3 skipped native | PASS |
| TypeScript compiles | `npx tsc --noEmit --project app/tsconfig.json` | No output (clean) | PASS |
| MeterData struct in C++ header | grep `struct MeterData` insert_chain_processor.h | Found at line 43 | PASS |
| getMeterLevels in bridge export | grep `getMeterLevels` native/src/addon.cpp | Found at line 50 | PASS |
| Effect reorder in UI | grep `reorder\|draggable\|onDragStart` mixer components | No matches | FAIL |
| Mixer height resizable | grep `handleMixerDividerDrag` App.tsx | Empty function body | FAIL |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| MIX-01 | 08-01-PLAN, 08-03-PLAN | Per-track channel strip with volume fader, pan knob, mute, and solo | SATISFIED | ChannelStrip.tsx: Fader + PanKnob + M/S/Arm buttons fully wired |
| MIX-02 | 08-01-PLAN, 08-03-PLAN | Master channel strip with volume fader and insert effect chain | SATISFIED | MasterStrip.tsx: volume fader via Fader + masterEffects via EffectSlotList |
| MIX-03 | 08-01-PLAN, 08-03-PLAN | Per-track insert effect chain with add, remove, reorder, and bypass per effect | PARTIAL | Add/remove/bypass: wired. Reorder: store method exists but no UI interaction exposes it |
| MIX-04 | 08-02-PLAN, 08-03-PLAN | Visual level meters on each channel strip showing real-time signal level | SATISFIED | Full pipeline: C++ atomics → bridge → IPC → preload → RAF polling → mixer-store → LevelMeter canvas |

**Orphaned requirements:** None. All four MIX requirements appear in at least one plan's `requirements` field and are accounted for above.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `app/src/renderer/App.tsx` | 37-40 | `handleMixerDividerDrag` is empty callback — SplitDivider drag does nothing | Warning | Mixer panel not resizable; always renders at fixed 300px |
| `app/src/renderer/components/mixer/EffectSlotList.tsx` | 42-44 | `onRemove={() => {}}`, `onBypass={() => {}}`, `onClick={() => {}}` on empty slot | Info | Expected for the empty "Add Effect" row which cannot be removed/bypassed; not a stub |
| `app/src/renderer/components/mixer/ChannelStrip.tsx` | 134-137 | `leftPeak={level?.peakL ?? 0}` same as leftLevel — no separate peak hold value | Info | Visual limitation: peak hold indicator coincides with bar top; store stores only peak, not separate RMS |

### Human Verification Required

#### 1. Mixer Panel Visual Layout

**Test:** Run `npm run dev`, click "Mixer" button in timeline toolbar
**Expected:** Mixer panel opens below timeline; channel strips render for each existing track (left-to-right, scrollable); Master strip pinned rightmost, wider with purple left border
**Why human:** Requires running Electron app; visual correctness cannot be verified from static code

#### 2. Level Meter Animation

**Test:** With mixer open, play audio using an instrument (e.g., PolySynth). Observe level meters.
**Expected:** Canvas 2D bars animate with green/amber/red gradient at approximately 30Hz; bars grow taller with louder signal
**Why human:** Requires live audio playback; C++ engine audio thread must be producing samples

#### 3. Interactive Controls

**Test:** Drag a volume fader up and down; double-click to reset; drag pan knob vertically; double-click to center
**Expected:** Fader dB readout updates continuously from -inf to +6dB; double-click snaps to 0dB; pan label changes (e.g., "50L", "C", "75R"); double-click returns to "C"
**Why human:** Requires pointer event interaction in running app

#### 4. Effect Slots Add/Remove/Bypass/Popover

**Test:** Click empty effect slot, choose "Reverb", verify slot appears; click bypass toggle; click filled slot for popover; right-click for context menu "Remove Effect"
**Expected:** All four operations function correctly with correct visual feedback
**Why human:** Requires interactive click/right-click in running app

#### 5. Plan 03 Task 3 Checkpoint

**Note:** Plan 03's Task 3 is a blocking `checkpoint:human-verify` gate that was not completed. The SUMMARY records this as "checkpoint pending". Human sign-off on the above items constitutes completing this checkpoint.

### Gaps Summary

Two functional gaps block complete goal achievement:

**Gap 1 — Effect slot reorder not wired (MIX-03 partial)**
MIX-03 requires "add, remove, reorder, and bypass per effect." The store correctly implements `reorderTrackEffect` and `reorderMasterEffect`, but no UI element (drag handles, onDragStart, onDrop) exposes reorder to the user. The `EffectSlotList` component has no `onReorder` prop and no drag-and-drop implementation. This is the sole remaining piece of MIX-03.

**Gap 2 — Mixer panel not resizable**
Plan 03's success criterion states "Panel resizable via split divider." The `SplitDivider` component is rendered between the timeline and mixer panel in `App.tsx`, but `handleMixerDividerDrag` is an empty function and `mixerHeight` is a hardcoded constant (`300`). The user cannot resize the mixer panel by dragging. This is a usability gap — the mixer is functional at the fixed height but doesn't behave like the piano roll panel which correctly uses `panelHeight` from its store.

Both gaps are isolated to the UI layer and do not affect the data layer (store, C++ engine, bridge, IPC) which is fully verified. All four requirements are at minimum partially satisfied. MIX-01, MIX-02, and MIX-04 are fully satisfied.

---

_Verified: 2026-03-29T16:02:00Z_
_Verifier: Claude (gsd-verifier)_
