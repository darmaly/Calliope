---
phase: 09-project-management-export
verified: 2026-03-29T23:30:00Z
status: gaps_found
score: 8/13 must-haves verified
re_verification: false
gaps:
  - truth: "C++ engine can offline-bounce the full mix to a WAV file"
    status: partial
    reason: "audio_exporter.cpp calls graph.getMasterBus().processBlock(blockBuffer, midiBuffer) twice per loop iteration (lines 172 and 181). The first call writes processed audio, then blockBuffer.clear() is called before the second call, which means output is always silence. Real data never makes it into the output buffer."
    artifacts:
      - path: "engine/src/audio_exporter.cpp"
        issue: "Duplicate processBlock calls at lines 172 and 181; buffer cleared between them (line 177-180 sets size and clears). Second call overwrites output with cleared buffer, meaning exported audio is silence."
    missing:
      - "Remove the duplicate processBlock call. Keep only one call to graph.getMasterBus().processBlock(blockBuffer, midiBuffer), then copy blockBuffer to output. The clearance+resize between calls nullifies the first render."
  - truth: "Export progress is reported via callback during bounce"
    status: partial
    reason: "Progress callback fires from within the offline bounce loop, but because the bounce produces silence (see above gap), a successful progress report would accompany an empty export. The wiring for the callback itself is correct, but the underlying data it represents is broken."
    artifacts:
      - path: "engine/src/audio_exporter.cpp"
        issue: "Dependent on the same duplicate-processBlock bug above."
    missing:
      - "Fix the duplicate processBlock issue; progress reporting wiring is otherwise correct."
  - truth: "C++ engine can encode the bounced audio to MP3 via LAME"
    status: partial
    reason: "LAME encoding implementation is correct and properly guarded with #ifdef CALLIOPE_HAS_LAME. However, the audio data fed to LAME is silence due to the duplicate-processBlock bug in offlineBounce(). The encoding logic itself is correct but operates on empty data."
    artifacts:
      - path: "engine/src/audio_exporter.cpp"
        issue: "Downstream of the offlineBounce() bug — writeMp3() is correct but receives silence."
    missing:
      - "Same fix as the bounce bug. writeMp3() itself does not need changes."
  - truth: "Export produces a valid audio file at the chosen path"
    status: failed
    reason: "Three separate blocking gaps prevent valid export files from being produced: (1) the C++ bridge functions ExportAudio, ExportStems, LoadProjectState are never registered in addon.cpp, so calling native.exportAudio() at runtime throws 'is not a function'; (2) even if the bridge were registered, the offline bounce produces silence due to the duplicate-processBlock bug; (3) TypeScript compilation fails with 8 errors because calliope.d.ts is missing the export API declarations."
    artifacts:
      - path: "native/src/addon.cpp"
        issue: "ExportAudio, ExportStems, LoadProjectState are not registered. bridge.h also missing these declarations. Only SaveProject and LoadProject are registered for Phase 9."
      - path: "app/src/renderer/types/calliope.d.ts"
        issue: "Missing declarations for exportAudio, exportStems, loadProjectState, onExportProgress, showExportPathDialog, onShowExportDialog, removeExportProgressListener, removeShowExportDialogListener."
      - path: "engine/src/audio_exporter.cpp"
        issue: "Offline bounce produces silence."
    missing:
      - "In native/src/bridge.h: add declarations for ExportAudio, ExportStems, LoadProjectState."
      - "In native/src/bridge.cpp: implement ExportAudio (TSFN+thread pattern, calls Engine::getInstance().getAudioExporter().exportMix()), ExportStems (calls exportStems()), and LoadProjectState (accepts JSON string, restores engine via ParameterRegistry)."
      - "In native/src/addon.cpp: register all three: exports.Set('exportAudio', ...), exports.Set('exportStems', ...), exports.Set('loadProjectState', ...)."
      - "In app/src/renderer/types/calliope.d.ts: add all missing export API methods to CalliopeAPI interface."
      - "In engine/src/audio_exporter.cpp: remove duplicate processBlock call and the spurious clear/resize between the two calls."
  - truth: "Stem export option produces separate WAV files per track in a stems/ subfolder"
    status: failed
    reason: "Same root cause as above — ExportStems bridge function is not registered in addon.cpp, so the IPC handler project:exportStems would call native.exportStems() which does not exist on the addon at runtime."
    artifacts:
      - path: "native/src/addon.cpp"
        issue: "ExportStems not registered."
    missing:
      - "Same bridge registration fix as in the 'Export produces a valid audio file' gap above."
human_verification:
  - test: "Verify export produces a non-silent audio file after bridge fix"
    expected: "WAV file at selected path contains audible audio when the project has MIDI notes on any track."
    why_human: "Requires running the app with audio engine initialized and playing back MIDI content to confirm offline bounce actually captures instrument output."
  - test: "Verify save/load round-trip preserves full state"
    expected: "After saving and loading a .ltproj file, all transport settings, instrument parameters, and mixer settings are restored to exactly the state at save time."
    why_human: "Requires interactive UI verification across all editable parameters — cannot be checked programmatically without running the full app."
  - test: "Verify autosave timer fires and saves without user intervention"
    expected: "After making a change, waiting 120 seconds, and confirming the .ltproj file's modified timestamp has updated, with the project title bar losing its asterisk."
    why_human: "Requires waiting 2 minutes with the app running — timing-dependent behavior."
---

# Phase 9: Project Management & Export Verification Report

**Phase Goal:** Users can save, load, and export their projects in multiple audio formats
**Verified:** 2026-03-29T23:30:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Project state saves to a JSON file on disk via engine serialization | VERIFIED | `ProjectSerializer::saveToFile` implemented with versioned JSON envelope; `SaveProject` bridge registered in addon.cpp; IPC handler `project:save` wired to `native.saveProject()`; preload exposes `projectSave()`. |
| 2 | Project state loads from a JSON file on disk and restores engine state | VERIFIED | `ProjectSerializer::loadFromFile` restores all transport, instrument, and effect parameters via ParameterRegistry setters; `LoadProject` bridge registered; IPC handler wired; preload exposes `projectLoad()`. |
| 3 | Autosave triggers at configurable interval (default 120 seconds) in the main process | VERIFIED | `autosaveTimer` using `setInterval(autosaveIntervalMs)` in `app/src/main/index.ts` lines 306-322; fires only when `autosaveEnabled && isDirty && filePath`. `project:setAutosave` IPC handler updates interval. |
| 4 | Save/load/autosave operations are accessible from the renderer via preload API | VERIFIED | `preload/index.ts` exposes `projectSave`, `projectSaveAs`, `projectLoad`, `projectNew`, `projectSetAutosave`, `projectGetAutosaveConfig`, `onProjectAutosaved`. All are typed in `calliope.d.ts`. |
| 5 | Project store tracks dirty state, file path, and autosave settings | VERIFIED | `project-store.ts` has `isDirty`, `filePath`, `autosaveEnabled`, `autosaveIntervalMs`, `lastSaved`, `lastAutosaved`; `initProjectDirtyTracking()` subscribes to command events to auto-mark dirty. |
| 6 | C++ engine can offline-bounce the full mix to a WAV file | FAILED | `audio_exporter.cpp` calls `getMasterBus().processBlock(blockBuffer, midiBuffer)` at line 172, then sets blockBuffer size and clears it (lines 171, 177-180), then calls `processBlock` again at line 181. The second call overwrites output with a cleared buffer, producing silence. Output buffer `output.copyFrom()` at line 184 copies from the second (cleared) buffer. |
| 7 | C++ engine can encode the bounced audio to MP3 via LAME | PARTIAL | `writeMp3()` correctly uses `lame_encode_buffer_interleaved_ieee_float` with proper init/flush/close. LAME CMake detection is in place. However, it receives silence from the broken `offlineBounce()`. Logic is correct, data is wrong. |
| 8 | C++ engine can write the bounced audio to FLAC | PARTIAL | `writeFlac()` correctly uses `juce::FlacAudioFormat`. Same silent-data issue from offlineBounce(). |
| 9 | C++ engine can render individual track stems as separate WAV files | PARTIAL | `exportStems()` and `offlineBounceSingleTrack()` implementations are structurally complete. MIDI events are filtered by trackId correctly. Same silent-data bug applies. Additionally, the `ExportStems` bridge function is never registered in `addon.cpp`, so the IPC pathway is broken before the C++ code is even reached. |
| 10 | Export progress is reported via callback during bounce | PARTIAL | Progress callback fires correctly in the bounce loop with `progress(pct)`. TSFN pattern is wired in the IPC handler at main/index.ts line 245. However, the bridge `ExportAudio` function is not registered in addon.cpp, so this path is never reachable at runtime. |
| 11 | User can open an Export dialog from File > Export menu | VERIFIED | `App.tsx` listens to `menu:export` via `onShowExportDialog` callback (line 36), calls `showExportDialog()` on project-store. `ExportDialog` renders conditionally when `exportDialogOpen === true`. |
| 12 | Export dialog offers WAV (16/24-bit), MP3 (bitrate selection), and FLAC format options | VERIFIED | `ExportDialog.tsx` has a format dropdown with 'wav16', 'wav24', 'mp3', 'flac' options. MP3 bitrate selector (128/192/256/320 kbps) renders conditionally when format === 'mp3'. Stems checkbox present. |
| 13 | Export errors show a toast notification with retry button | VERIFIED | `Toast.tsx` renders with `onRetry` callback; `ExportDialog.tsx` catch block calls `showToast({ type: 'error', retryFn: doExport })`; error toast auto-stays open (no auto-dismiss when retryFn present). |

**Score:** 8/13 truths verified (5 failed or partial)

---

### Required Artifacts

| Artifact | Provided | Status | Details |
|----------|----------|--------|---------|
| `engine/include/calliope/project_serializer.h` | C++ project save/load with versioned JSON | VERIFIED | `saveToFile`, `loadFromFile`, `getFileVersion` declared. Substantive. |
| `engine/src/project_serializer.cpp` | File I/O with version envelope | VERIFIED | 99 lines; full implementation of envelope wrapping/unwrapping and file I/O. |
| `engine/include/calliope/audio_exporter.h` | AudioExporter class declaration | VERIFIED | All methods declared: `exportMix`, `exportStems`, `parseMidiEventsJson`, `offlineBounce`, write methods. |
| `engine/src/audio_exporter.cpp` | Offline bounce, WAV/FLAC/MP3 write | STUB (partial) | 426 lines, substantive implementation, but contains a logic bug that causes exported audio to be silence. Not a placeholder — a real bug. |
| `engine/tests/test_audio_exporter.cpp` | Catch2 test scaffold | VERIFIED | 4 TEST_CASE blocks for `parseMidiEventsJson` with 539+ assertions. Integration test for WAV export also present. |
| `native/src/bridge.h` | ExportAudio, ExportStems, LoadProjectState declarations | MISSING | Only `SaveProject` and `LoadProject` are declared for Phase 9. `ExportAudio`, `ExportStems`, `LoadProjectState` are absent. |
| `native/src/bridge.cpp` | ExportAudio, ExportStems, LoadProjectState implementations | MISSING | Only `SaveProject` and `LoadProject` implemented. No export bridge functions exist. |
| `native/src/addon.cpp` | Registration of export bridge functions | MISSING | Only `saveProject` and `loadProject` registered. `exportAudio`, `exportStems`, `loadProjectState` not registered. |
| `app/src/renderer/stores/project-store.ts` | Zustand store with dirty tracking and export state | VERIFIED | `isDirty`, `filePath`, `autosaveEnabled`, `autosaveIntervalMs`, `exportDialogOpen`, `exportProgress`, `toastMessage`; all actions implemented; `initProjectDirtyTracking()` wires command events. |
| `app/src/renderer/components/export/ExportDialog.tsx` | Modal with format selection and export trigger | VERIFIED | 254 lines; format dropdown, MP3 bitrate selector, stems checkbox, browse button, MIDI event collection with camelCase fields, full error handling. |
| `app/src/renderer/components/export/ExportProgress.tsx` | Progress bar overlay | VERIFIED | 51 lines; progress bar driven by `exportProgress` from project-store; auto-dismiss after completion. |
| `app/src/renderer/components/shared/Toast.tsx` | Toast notification with retry | VERIFIED | 54 lines; error/success variants, auto-dismiss after 5s unless error+retry, retry callback, X button. |
| `app/src/renderer/types/calliope.d.ts` | TypeScript type declarations for export API | PARTIAL | Has save/load declarations but missing all export API types: `exportAudio`, `exportStems`, `loadProjectState`, `onExportProgress`, `showExportPathDialog`, `onShowExportDialog`, `removeExportProgressListener`, `removeShowExportDialogListener`. TypeScript compilation fails with 8 errors. |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| `app/src/preload/index.ts` | `app/src/main/index.ts` | `ipcRenderer.invoke('project:save')` | WIRED | Handler at main/index.ts line 140; calls `native.saveProject(filePath)`. |
| `native/src/bridge.cpp` | `engine/include/calliope/project_serializer.h` | `ProjectSerializer::saveToFile` | WIRED | Line 924 in bridge.cpp calls `calliope::ProjectSerializer::saveToFile(...)`. |
| `app/src/preload/index.ts` | `app/src/main/index.ts` | `ipcRenderer.invoke('project:export')` | WIRED | IPC handler exists at main/index.ts line 228; calls `native.exportAudio(...)`. |
| `native.exportAudio` | C++ ExportAudio bridge | `addon.cpp registration` | NOT_WIRED | `native.exportAudio()` in main/index.ts calls the native addon method, but `ExportAudio` is not registered in addon.cpp. Runtime call will throw TypeError. |
| `engine/src/audio_exporter.cpp` | Engine AudioProcessorGraph | `processBlock()` in offline loop | PARTIAL | `processBlock` is called (line 172, 181) but called twice with a buffer clear between calls. First render is overwritten by cleared second render. |
| `engine/src/audio_exporter.cpp` | LAME library | `lame_encode_buffer_interleaved_ieee_float` | WIRED | Line 315 — LAME call present and correctly structured. Guard at line 271. |
| `app/src/renderer/components/export/ExportDialog.tsx` | `window.calliope.exportAudio()` | preload API call on export submit | WIRED | Line 115 calls `window.calliope.exportAudio({...})`. But calliope.d.ts missing the type declaration causes 8 TS compile errors. |
| `app/src/renderer/components/export/ExportProgress.tsx` | `window.calliope.onExportProgress()` | IPC progress event listener | ORPHANED | ExportProgress.tsx does not call `onExportProgress` directly — it reads from project-store. The `onExportProgress` subscription is set up in ExportDialog.tsx handleExport (line 111). Wiring is present but different component than plan specified. |

---

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|-------------------|--------|
| `ExportDialog.tsx` | `outputPath` | `showExportPathDialog()` IPC call | Yes — returns actual path from Electron dialog | FLOWING |
| `ExportDialog.tsx` | `midiEvents` | `collectMidiEvents()` iterating timeline-store clips | Real timeline data | FLOWING |
| `ExportProgress.tsx` | `exportProgress` | `project-store.exportProgress` (set by ExportDialog on progress events) | Real 0-1 float from C++ progress callback | HOLLOW — bridge not registered so progress events never fire |
| `project-store.ts` | `isDirty` | `onCommandEvent` subscription fires `markDirty()` | Real command events from engine | FLOWING |
| `audio_exporter.cpp` | `output` buffer | `offlineBounce()` → `processBlock()` loop | Bug: produces silence due to duplicate processBlock / buffer clear | DISCONNECTED |

---

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| TypeScript compiles without errors | `pnpm exec tsc --noEmit` | 8 errors: `exportAudio`, `exportStems`, `onExportProgress`, `showExportPathDialog`, `onShowExportDialog`, `removeExportProgressListener` × 2, `removeShowExportDialogListener` not found on CalliopeAPI | FAIL |
| Export bridge registered in native addon | `grep ExportAudio native/src/addon.cpp` | No match — only `saveProject` and `loadProject` registered | FAIL |
| processBlock called once per loop iteration | `grep -n processBlock engine/src/audio_exporter.cpp` | Called at line 172 and again at line 181 with buffer clear between | FAIL |
| LAME encode function present | `grep lame_encode engine/src/audio_exporter.cpp` | `lame_encode_buffer_interleaved_ieee_float` at line 315, `lame_encode_flush` at line 335 | PASS |
| SaveProject registered in addon | `grep saveProject native/src/addon.cpp` | Line 66-67 registered | PASS |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| PROJ-01 | 09-01 | Project save to file and load from file with full state restoration | SATISFIED | `ProjectSerializer::saveToFile/loadFromFile` implemented; `SaveProject`/`LoadProject` bridge registered; full ParameterRegistry-based state restoration on load; IPC handlers and preload API complete. |
| PROJ-02 | 09-01 | Autosave at configurable interval (default every 2 minutes) | SATISFIED | `autosaveTimer` in main/index.ts fires every `autosaveIntervalMs` (default 120000ms); saves when `autosaveEnabled && isDirty && currentFilePath`; interval configurable via `project:setAutosave` IPC. |
| PROJ-04 | 09-02, 09-03 | Export final mix to WAV (uncompressed, 16/24-bit) | BLOCKED | `AudioExporter::writeWav()` implementation correct; ExportDialog triggers it; but bridge function `ExportAudio` not registered in addon.cpp; and `offlineBounce()` produces silence due to duplicate processBlock bug. |
| PROJ-05 | 09-02, 09-03 | Export final mix to MP3 (configurable bitrate) | BLOCKED | LAME encoding correct in `writeMp3()`; CMake detection present; but same two blockers: missing bridge registration and silent bounce. |
| PROJ-06 | 09-02, 09-03 | Export final mix to FLAC (lossless) | BLOCKED | `writeFlac()` using JUCE FlacAudioFormat is correct; same two blockers apply. |
| PROJ-07 | 09-02, 09-03 | Export individual track stems as separate WAV files | BLOCKED | `exportStems()` logic filters MIDI by trackId and writes per-track WAVs; `ExportStems` bridge not registered in addon.cpp; silent bounce bug; and stems checkbox in ExportDialog correctly wired to `exportStems()` IPC call. |

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `engine/src/audio_exporter.cpp` | 171-181 | Double processBlock call with buffer clear between: `processBlock()` at 172, then `setSize()+clear()` at 177-180, then `processBlock()` again at 181. Only the second (cleared) call's output is copied to `output`. | Blocker | All exported audio is silence. WAV, FLAC, and MP3 exports write valid files containing no audio. |
| `native/src/bridge.h` | 41-43 | Missing declarations for ExportAudio, ExportStems, LoadProjectState | Blocker | The TypeScript layer declares and calls these functions, but they don't exist in the native module. Runtime `TypeError: native.exportAudio is not a function` on any export attempt. |
| `app/src/renderer/types/calliope.d.ts` | 86 (end) | Missing export API types: exportAudio, exportStems, loadProjectState, onExportProgress, showExportPathDialog, onShowExportDialog, removeExportProgressListener, removeShowExportDialogListener | Blocker | TypeScript build fails with 8 errors. App cannot be compiled. |

---

### Human Verification Required

#### 1. Export produces audible non-silent audio (after bug fixes)

**Test:** After applying fixes — (a) remove duplicate processBlock call in audio_exporter.cpp, (b) register ExportAudio/ExportStems in addon.cpp, (c) add type declarations to calliope.d.ts — start the app, add MIDI notes to a track, open File > Export, choose WAV 24-bit, click Export, and open the resulting file in an audio player.
**Expected:** The exported WAV file plays back recognizable audio corresponding to the MIDI notes.
**Why human:** Requires running the full audio engine stack with actual instrument synthesis and verifying audible output — cannot be checked with file existence or byte counts alone.

#### 2. Save/load round-trip preserves full state

**Test:** Start the app. Set BPM to 140, adjust PolySynth filter cutoff to 3000 Hz, add a reverb effect. Save as `test.ltproj`. Quit and reopen. Load `test.ltproj`. Verify all three settings are restored.
**Expected:** BPM = 140, filter cutoff = 3000 Hz, reverb effect present in the chain.
**Why human:** Requires interactive verification across all instrument/effect parameter types. The ParameterRegistry restoration code is extensive but each individual parameter path cannot be verified programmatically without running the engine.

#### 3. Autosave timer fires and saves without user intervention

**Test:** Open app with a new project, make an edit (change BPM), wait 2 minutes (or reduce autosave interval via dev tools for testing). Verify the title bar asterisk disappears and a `.ltproj` file appears on disk.
**Expected:** Autosave creates the file with a timestamped name, title bar reflects clean state.
**Why human:** Time-dependent behavior requiring the full running application.

---

## Gaps Summary

Phase 9 delivers a complete save/load foundation (PROJ-01, PROJ-02) but is blocked on all four export requirements (PROJ-04, PROJ-05, PROJ-06, PROJ-07) due to three compounding defects.

**Gap 1 — Missing C++ bridge registration (Blocker):** The `ExportAudio`, `ExportStems`, and `LoadProjectState` C++ bridge functions were implemented in `bridge.cpp` in the SUMMARY's claimed commit `9a0dba2`, but they are absent from the actual `bridge.h` declarations and `addon.cpp` registration. Only the TypeScript type stubs in `native-bridge.ts` were written. Any call to `native.exportAudio()`, `native.exportStems()`, or `native.loadProjectState()` from the Electron main process will throw a TypeError at runtime. This gap exists in the bridge layer between Plans 02 and 03.

**Gap 2 — Silent offline bounce (Blocker):** `offlineBounce()` in `audio_exporter.cpp` calls `graph.getMasterBus().processBlock(blockBuffer, midiBuffer)` at line 172, then calls `blockBuffer.setSize(2, samplesThisBlock, false, false, true)` and `blockBuffer.clear()` at lines 171/177-180 (these lines reorganize and clear the buffer), and then calls `processBlock` again at line 181. The `output.copyFrom()` at line 184 reads from the second (cleared) pass, so all exported audio is silence. This is a logic error introduced in Plan 02 Task 1.

**Gap 3 — TypeScript type declarations missing (Blocker):** `calliope.d.ts` was not updated with the export API additions from Plans 02 and 03. Eight properties on `CalliopeAPI` are used in ExportDialog.tsx and App.tsx but not declared in the type file. `pnpm exec tsc --noEmit` reports 8 errors. The app cannot compile.

**Root cause of Gaps 1 and 3:** Plan 02 stated it would add bridge registrations and type declarations but the actual bridge functions and type updates were not committed to the files. Plan 03 noted this as a "Rule 3 deviation" and said it fixed the missing bridge functions, but the fix only updated the TypeScript layer (native-bridge.ts, preload, main/index.ts) — the C++ side (bridge.h, bridge.cpp, addon.cpp) was still not updated.

To close these gaps: register ExportAudio/ExportStems/LoadProjectState in the C++ bridge; fix the duplicate processBlock; and add the export API type declarations to calliope.d.ts. After those fixes, the complete export pipeline should function end-to-end with human verification.

---

_Verified: 2026-03-29T23:30:00Z_
_Verifier: Claude (gsd-verifier)_
