---
phase: 09-project-management-export
verified: 2026-03-29T23:55:00Z
status: human_needed
score: 13/13 must-haves verified
re_verification: true
  previous_status: gaps_found
  previous_score: 8/13
  gaps_closed:
    - "ExportAudio, ExportStems, LoadProjectState declared in bridge.h, implemented in bridge.cpp, registered in addon.cpp"
    - "Duplicate processBlock call removed from audio_exporter.cpp — single call at line 173, output correctly copied after"
    - "8 export API declarations added to calliope.d.ts — TypeScript compiles with 0 errors"
  gaps_remaining: []
  regressions: []
human_verification:
  - test: "Export produces audible non-silent audio"
    expected: "WAV file at selected path contains audible audio corresponding to MIDI notes in the project."
    why_human: "Requires running the full audio engine stack with actual instrument synthesis and verifying audible output — cannot be confirmed with file size or byte counts alone."
  - test: "Save/load round-trip preserves full state"
    expected: "After saving and loading a .ltproj file, all transport settings, instrument parameters, and mixer settings are restored exactly."
    why_human: "Requires interactive UI verification across all editable parameter types with the full app running."
  - test: "Autosave timer fires and saves without user intervention"
    expected: "After making an edit, the .ltproj file's modified timestamp updates within the configured interval (default 120 seconds) and the title bar asterisk disappears."
    why_human: "Timing-dependent behavior requiring the full running application."
---

# Phase 9: Project Management & Export Verification Report

**Phase Goal:** Users can save, load, and export their projects in multiple audio formats
**Verified:** 2026-03-29T23:55:00Z
**Status:** human_needed
**Re-verification:** Yes — after gap closure (previous score: 8/13, previous status: gaps_found)

---

## Re-verification Summary

All three gaps from the initial verification are confirmed closed:

1. **C++ bridge registration** — `ExportAudio`, `ExportStems`, and `LoadProjectState` are now declared in `native/src/bridge.h` (lines 46-48), fully implemented in `native/src/bridge.cpp` (lines 1041-1259), and registered in `native/src/addon.cpp` (lines 72-77). Each uses the TSFN+detached-thread async pattern matching the project's existing bridge conventions.

2. **Silent offline bounce** — `audio_exporter.cpp` now has a single `graph.getMasterBus().processBlock(blockBuffer, midiBuffer)` call at line 173. The spurious second call and the buffer-clear between calls have been removed. The `output.copyFrom()` block at lines 176-178 follows immediately after the single render, correctly capturing processed audio.

3. **TypeScript type declarations** — `calliope.d.ts` now contains all 8 export API declarations (lines 88-105): `exportAudio`, `exportStems`, `loadProjectState`, `onExportProgress`, `removeExportProgressListener`, `showExportPathDialog`, `onShowExportDialog`, `removeShowExportDialogListener`. `pnpm exec tsc --noEmit` exits with 0 errors.

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Project state saves to a JSON file on disk via engine serialization | VERIFIED | `ProjectSerializer::saveToFile` implemented; `SaveProject` bridge registered in addon.cpp line 66; `project:save` IPC handler wired; preload exposes `projectSave()`. |
| 2 | Project state loads from a JSON file on disk and restores engine state | VERIFIED | `ProjectSerializer::loadFromFile` restores all parameters via ParameterRegistry; `LoadProject` bridge registered; IPC handler and preload API complete. |
| 3 | Autosave triggers at configurable interval (default 120 seconds) in the main process | VERIFIED | `autosaveTimer` using `setInterval(autosaveIntervalMs)` in `app/src/main/index.ts` lines 302-309; fires only when `autosaveEnabled && isDirty && currentFilePath`; configurable via `project:setAutosave` IPC. |
| 4 | Save/load/autosave operations are accessible from the renderer via preload API | VERIFIED | `preload/index.ts` exposes `projectSave`, `projectSaveAs`, `projectLoad`, `projectNew`, `projectSetAutosave`, `projectGetAutosaveConfig`, `onProjectAutosaved`. All typed in `calliope.d.ts`. |
| 5 | Project store tracks dirty state, file path, and autosave settings | VERIFIED | `project-store.ts` has `isDirty`, `filePath`, `autosaveEnabled`, `autosaveIntervalMs`, `exportDialogOpen`, `exportProgress`; `initProjectDirtyTracking()` subscribes to command events to mark dirty. |
| 6 | C++ engine can offline-bounce the full mix to a WAV file | VERIFIED | `offlineBounce()` now has single `processBlock` call at line 173, followed immediately by `output.copyFrom()`. Duplicate call and intervening clear removed. Correct data flows to output. |
| 7 | C++ engine can encode the bounced audio to MP3 via LAME | VERIFIED | `writeMp3()` correctly uses `lame_encode_buffer_interleaved_ieee_float`; LAME CMake detection present; `offlineBounce()` now produces real audio data that feeds into encoding. |
| 8 | C++ engine can write the bounced audio to FLAC | VERIFIED | `writeFlac()` uses `juce::FlacAudioFormat` correctly; same real-data fix from offlineBounce() applies. |
| 9 | C++ engine can render individual track stems as separate WAV files | VERIFIED | `exportStems()` and `offlineBounceSingleTrack()` structurally complete with MIDI filtering by trackId; `ExportStems` bridge now registered in addon.cpp line 75; `project:exportStems` IPC handler wired at main/index.ts line 253. |
| 10 | Export progress is reported via callback during bounce | VERIFIED | Progress TSFN fires from bounce loop; `ExportAudio` bridge registered; IPC handler at main/index.ts line 245 sends `project:exportProgress` to renderer; `ExportDialog.tsx` wires `onExportProgress` to `setExportProgress` on project-store. |
| 11 | User can open an Export dialog from File > Export menu | VERIFIED | `App.tsx` listens to `menu:export` via `onShowExportDialog` callback; `ExportDialog` renders conditionally on `exportDialogOpen`. |
| 12 | Export dialog offers WAV (16/24-bit), MP3 (bitrate selection), and FLAC format options | VERIFIED | `ExportDialog.tsx` has format dropdown with 'wav16', 'wav24', 'mp3', 'flac'; MP3 bitrate selector (128/192/256/320 kbps) renders when format === 'mp3'; stems checkbox present. |
| 13 | Export errors show a toast notification with retry button | VERIFIED | `Toast.tsx` renders with `onRetry` callback; ExportDialog catch block calls `showToast({ type: 'error', retryFn: doExport })`; error toast stays open when retryFn present. |

**Score:** 13/13 truths verified

---

### Required Artifacts

| Artifact | Provided | Status | Details |
|----------|----------|--------|---------|
| `engine/include/calliope/project_serializer.h` | C++ project save/load with versioned JSON | VERIFIED | `saveToFile`, `loadFromFile`, `getFileVersion` declared. Substantive. |
| `engine/src/project_serializer.cpp` | File I/O with version envelope | VERIFIED | Full implementation of envelope wrapping/unwrapping and file I/O. |
| `engine/include/calliope/audio_exporter.h` | AudioExporter class declaration | VERIFIED | All methods declared: `exportMix`, `exportStems`, `parseMidiEventsJson`, `offlineBounce`, write methods. |
| `engine/src/audio_exporter.cpp` | Offline bounce, WAV/FLAC/MP3 write | VERIFIED | Single processBlock call at line 173; output correctly copied; LAME and FLAC write implementations intact. Bug from initial verification is resolved. |
| `engine/tests/test_audio_exporter.cpp` | Catch2 test scaffold | VERIFIED | 4 TEST_CASE blocks with assertions; integration test for WAV export present. |
| `native/src/bridge.h` | ExportAudio, ExportStems, LoadProjectState declarations | VERIFIED | Lines 46-48: all three declared. Previously missing. |
| `native/src/bridge.cpp` | ExportAudio, ExportStems, LoadProjectState implementations | VERIFIED | Lines 1041-1259: TSFN+thread implementations; `ExportAudio` calls `exporter.exportMix()`; `ExportStems` calls `exporter.exportStems()`; `LoadProjectState` parses JSON and restores ParameterRegistry. Previously missing. |
| `native/src/addon.cpp` | Registration of export bridge functions | VERIFIED | Lines 72-77: `exportAudio`, `exportStems`, `loadProjectState` registered. Previously missing. |
| `app/src/renderer/stores/project-store.ts` | Zustand store with dirty tracking and export state | VERIFIED | `isDirty`, `filePath`, `autosaveEnabled`, `autosaveIntervalMs`, `exportDialogOpen`, `exportProgress`; `initProjectDirtyTracking()` wires command events. |
| `app/src/renderer/components/export/ExportDialog.tsx` | Modal with format selection and export trigger | VERIFIED | Format dropdown, MP3 bitrate selector, stems checkbox, browse button, full error handling, progress subscription via `onExportProgress`. |
| `app/src/renderer/components/export/ExportProgress.tsx` | Progress bar overlay | VERIFIED | Progress bar driven by `exportProgress` from project-store; auto-dismiss after completion. |
| `app/src/renderer/components/shared/Toast.tsx` | Toast notification with retry | VERIFIED | Error/success variants, auto-dismiss, retry callback. |
| `app/src/renderer/types/calliope.d.ts` | TypeScript type declarations for export API | VERIFIED | All 8 export API declarations present (lines 88-105). `pnpm exec tsc --noEmit` exits 0. Previously partial. |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| `app/src/preload/index.ts` | `app/src/main/index.ts` | `ipcRenderer.invoke('project:save')` | WIRED | Handler at main/index.ts; calls `native.saveProject(filePath)`. |
| `native/src/bridge.cpp` | `engine/include/calliope/project_serializer.h` | `ProjectSerializer::saveToFile` | WIRED | bridge.cpp calls `calliope::ProjectSerializer::saveToFile(...)`. |
| `app/src/preload/index.ts` | `app/src/main/index.ts` | `ipcRenderer.invoke('project:export')` | WIRED | IPC handler at main/index.ts line 228; calls `native.exportAudio(...)`. |
| `native.exportAudio` | C++ ExportAudio bridge | `addon.cpp registration` | WIRED | `ExportAudio` registered at addon.cpp line 72. Previously NOT_WIRED. |
| `native.exportStems` | C++ ExportStems bridge | `addon.cpp registration` | WIRED | `ExportStems` registered at addon.cpp line 75. Previously NOT_WIRED. |
| `native.loadProjectState` | C++ LoadProjectState bridge | `addon.cpp registration` | WIRED | `LoadProjectState` registered at addon.cpp line 77. Previously NOT_WIRED. |
| `engine/src/audio_exporter.cpp` | Engine AudioProcessorGraph | `processBlock()` in offline loop | WIRED | Single `processBlock` call at line 173; output copied at lines 176-178. Previously PARTIAL. |
| `engine/src/audio_exporter.cpp` | LAME library | `lame_encode_buffer_interleaved_ieee_float` | WIRED | LAME call at line 315, correctly structured. |
| `ExportDialog.tsx` | `window.calliope.exportAudio()` | preload API call on export submit | WIRED | Line 115 calls `window.calliope.exportAudio({...})`; type declaration present; TS compiles. Previously PARTIAL. |
| `ExportDialog.tsx` | `window.calliope.onExportProgress()` | IPC progress listener → project-store | WIRED | Line 111 subscribes to progress events, calls `setExportProgress(percent)` which flows to `ExportProgress.tsx`. |

---

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|-------------------|--------|
| `ExportDialog.tsx` | `outputPath` | `showExportPathDialog()` IPC call → Electron dialog | Yes — returns actual path | FLOWING |
| `ExportDialog.tsx` | `midiEvents` | `collectMidiEvents()` iterating timeline-store clips | Real timeline data | FLOWING |
| `ExportProgress.tsx` | `exportProgress` | `project-store.exportProgress` set by ExportDialog on `onExportProgress` events; events fired by C++ TSFN progress callback | Real 0-1 float from C++ via TSFN; bridge now registered so events can fire | FLOWING (was HOLLOW) |
| `project-store.ts` | `isDirty` | `onCommandEvent` subscription fires `markDirty()` | Real command events from engine | FLOWING |
| `audio_exporter.cpp` | `output` buffer | `offlineBounce()` → single `processBlock()` → `output.copyFrom()` | Real rendered audio from AudioProcessorGraph | FLOWING (was DISCONNECTED) |

---

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| TypeScript compiles without errors | `pnpm exec tsc --noEmit` | No output; exit 0 | PASS (was FAIL) |
| Export bridge registered in native addon | grep ExportAudio native/src/addon.cpp | Line 72-73 registered | PASS (was FAIL) |
| ExportStems bridge registered | grep ExportStems native/src/addon.cpp | Line 74-75 registered | PASS (was FAIL) |
| processBlock called once per loop iteration | grep -n processBlock engine/src/audio_exporter.cpp | Single call at line 173 only | PASS (was FAIL) |
| LAME encode function present | grep lame_encode engine/src/audio_exporter.cpp | `lame_encode_buffer_interleaved_ieee_float` at line 315 | PASS |
| SaveProject registered in addon | grep saveProject native/src/addon.cpp | Lines 66-67 registered | PASS |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| PROJ-01 | 09-01 | Project save to file and load from file with full state restoration | SATISFIED | `ProjectSerializer::saveToFile/loadFromFile` implemented; bridge registered; IPC handlers and preload complete. |
| PROJ-02 | 09-01 | Autosave at configurable interval (default every 2 minutes) | SATISFIED | `autosaveTimer` fires every `autosaveIntervalMs` (default 120000ms); saves when `autosaveEnabled && isDirty && currentFilePath`. |
| PROJ-04 | 09-02, 09-03 | Export final mix to WAV (uncompressed, 16/24-bit) | SATISFIED | `AudioExporter::writeWav()` correct; `ExportAudio` bridge registered; `offlineBounce()` produces real audio data. Pending human runtime verification. |
| PROJ-05 | 09-02, 09-03 | Export final mix to MP3 (configurable bitrate) | SATISFIED | `writeMp3()` LAME encoding correct; bridge registered; real audio data flows from fixed bounce. Pending human runtime verification. |
| PROJ-06 | 09-02, 09-03 | Export final mix to FLAC (lossless) | SATISFIED | `writeFlac()` using JUCE FlacAudioFormat correct; bridge registered; real audio data flows. Pending human runtime verification. |
| PROJ-07 | 09-02, 09-03 | Export individual track stems as separate WAV files | SATISFIED | `exportStems()` filters MIDI by trackId; `ExportStems` bridge registered; stems checkbox in ExportDialog wired to `project:exportStems` IPC. Pending human runtime verification. |

---

### Anti-Patterns Found

No blockers found. All three previously-identified blocker patterns have been resolved:

- Duplicate processBlock call removed from `audio_exporter.cpp`
- Missing bridge declarations added to `bridge.h`, `bridge.cpp`, `addon.cpp`
- Missing type declarations added to `calliope.d.ts`

No new anti-patterns introduced.

---

### Human Verification Required

#### 1. Export produces audible non-silent audio

**Test:** Start the app, add MIDI notes to a track with PolySynth or BassSynth, open File > Export, choose WAV 24-bit, click Export, and open the resulting file in an audio player.
**Expected:** The exported WAV file plays back recognizable audio corresponding to the MIDI notes. File size should be substantially larger than a header-only empty file (a 4-beat 44.1kHz WAV should be several MB).
**Why human:** Requires running the full audio engine stack with actual instrument synthesis. The C++ fix is structurally correct but cannot be verified without the engine synthesizing actual sound through the graph.

#### 2. Save/load round-trip preserves full state

**Test:** Start the app. Set BPM to 140, adjust PolySynth filter cutoff, add a reverb effect. Save as `test.ltproj`. Quit and reopen. Load `test.ltproj`. Verify all settings are restored.
**Expected:** BPM, filter cutoff, and reverb effect present exactly as saved.
**Why human:** Requires interactive verification across all instrument/effect parameter types with the full running app.

#### 3. Autosave timer fires and saves without user intervention

**Test:** Open app with a new project, make an edit, wait 2 minutes (or temporarily reduce `autosaveIntervalMs`). Verify the title bar asterisk disappears and a `.ltproj` file appears on disk.
**Expected:** Autosave creates the file; title bar reflects clean state.
**Why human:** Time-dependent behavior requiring the full running application.

---

## Gaps Summary

All automated gaps from the initial verification are closed. Phase 9 now delivers a complete and wired implementation across all six requirements (PROJ-01, PROJ-02, PROJ-04, PROJ-05, PROJ-06, PROJ-07). The three prior blockers — missing C++ bridge registration, silent offline bounce, and missing TypeScript declarations — have each been verified resolved in the actual codebase.

The remaining open items are runtime behaviors that require human verification with the application running: audio engine output quality, state round-trip fidelity, and autosave timing. These are not code gaps but operational confirmations.

---

_Verified: 2026-03-29T23:55:00Z_
_Verifier: Claude (gsd-verifier)_
