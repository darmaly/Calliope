# Phase 9: Project Management & Export - Research

**Researched:** 2026-03-29
**Domain:** Project file I/O, audio export/encoding, Electron file dialog and IPC, JUCE offline rendering
**Confidence:** HIGH

## Summary

Phase 9 adds project save/load, autosave, and audio export (WAV/MP3/FLAC + stems) to the Calliope DAW. The existing codebase already has strong foundations: `ProjectState` with full JSON serialization/deserialization, `snapshotFromEngine()` for capturing C++ engine state, and the `juce_audio_formats` module for WAV/AIFF/FLAC writing. The primary new work divides into three domains: (1) file I/O orchestration in the Electron main process coordinating Zustand UI state with C++ engine state, (2) offline bounce rendering in C++ that drives the `AudioProcessorGraph` without real-time audio device, and (3) MP3 encoding via LAME library integration into the CMake build.

The project file format is JSON with `.calliope` extension per CONTEXT.md decisions. The complete project state is a union of C++ engine state (transport, instruments, effects, mixer) and Zustand UI state (tracks, clips, MIDI notes, mixer volumes/pans). Save must capture both; load must restore both. Audio export requires an offline render loop that calls `processBlock()` on the AudioProcessorGraph in a tight loop writing to `AudioFormatWriter`, then optionally transcodes to MP3 via LAME.

**Primary recommendation:** Implement save/load as main-process IPC handlers that serialize both Zustand state (sent from renderer) and C++ engine state (from native bridge) into a single JSON file. Implement export as a C++ offline bounce function exposed via a new bridge function, with progress callback via ThreadSafeFunction.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Project file format is JSON with `.calliope` extension -- human-readable, consistent with ProjectState JSON serialization from Phase 3
- Save/load via File > Save / File > Open menu items plus Ctrl+S / Ctrl+O keyboard shortcuts
- Autosave writes to same directory as project file with `.calliope.autosave` suffix
- Full DAW state restoration: tracks, clips, MIDI notes, mixer settings (volume/pan/effects), transport position -- everything in ProjectState
- Export workflow uses a modal dialog with format selection, quality options, and file path picker
- Export processing is offline bounce through C++ engine (faster than real-time) with progress bar
- MP3 encoding via LAME library in C++ engine (LGPL-compatible, per CLAUDE.md stack spec)
- Stem export produces one WAV per track in a `stems/` subfolder next to the export file
- Autosave interval defaults to 2 minutes, configurable in settings
- Unsaved changes shown via title bar asterisk (*) and "Unsaved changes" warning on window close
- Export errors displayed as toast notification with error message and retry button
- Project name is "Calliope" -- file extension is `.calliope`, not `.ltp`

### Claude's Discretion
- Specific modal dialog layout and styling
- Progress bar animation style
- Settings UI for autosave interval configuration
- File picker default directory handling

### Deferred Ideas (OUT OF SCOPE)
- Rename "LuneyTunes" to "Calliope" across all project documentation (tracked separately, not part of Phase 9 scope)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PROJ-01 | Project save to file and load from file with full state restoration | ProjectState.toJson()/fromJson() handles engine state; main process IPC handlers coordinate UI state merge; Electron dialog API for file pickers |
| PROJ-02 | Autosave at configurable interval (default every 2 minutes) | setInterval in main process with configurable timer; writes to `.calliope.autosave` path; Zustand store for dirty tracking |
| PROJ-04 | Export final mix to WAV (uncompressed, 16/24-bit) | JUCE WavAudioFormat with AudioFormatWriter; offline bounce loop drives AudioProcessorGraph.processBlock() |
| PROJ-05 | Export final mix to MP3 (configurable bitrate) | LAME 3.100 via brew, CMake find_library; lame_encode_buffer_interleaved after WAV bounce |
| PROJ-06 | Export final mix to FLAC (lossless) | JUCE FlacAudioFormat built-in via juce_audio_formats module |
| PROJ-07 | Export individual track stems as separate WAV files | Render each track's graph chain independently by soloing/isolating individual tracks in offline bounce |
</phase_requirements>

## Standard Stack

### Core (already in project)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE juce_audio_formats | 8.0.12 | WAV/FLAC writing via AudioFormatWriter | Already linked. WavAudioFormat and FlacAudioFormat handle all lossless export. |
| Electron dialog API | 35.x | Native save/open/export file pickers | Built into Electron. dialog.showSaveDialog() / dialog.showOpenDialog() |
| Zustand | 5.x | UI state management + dirty tracking | Already in project. Add project-store for save/load/dirty state. |
| node-addon-api | 8.7.0 | Bridge for new export/import native functions | Already in project. Existing TSFN+thread pattern. |

### New (to add)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| LAME | 3.100 | MP3 encoding | Export to MP3. Install via `brew install lame` (macOS), `apt install libmp3lame-dev` (Linux). Link in CMake. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| LAME C library | ffmpeg CLI subprocess | Adds massive external dependency; LAME is lightweight and LGPL-compatible per CLAUDE.md |
| JSON project file | Binary/protobuf format | JSON is human-readable and consistent with existing ProjectState serialization |
| Offline bounce in C++ | Web Audio API rendering | Engine is C++; rendering there avoids double architecture and leverages existing AudioProcessorGraph |

## Architecture Patterns

### Project File Structure
```json
{
  "version": 1,
  "appVersion": "0.1.0",
  "savedAt": "2026-03-29T12:00:00Z",
  "engine": {
    "transport": { ... },
    "metronome": { ... },
    "masterBus": { ... },
    "audioConfig": { ... },
    "instruments": { ... },
    "effectChains": [ ... ]
  },
  "ui": {
    "tracks": [ ... ],
    "clips": { ... },
    "mixer": {
      "trackVolumes": { ... },
      "trackPans": { ... },
      "trackEffects": { ... },
      "masterVolume": 1.0,
      "masterPan": 0.0,
      "masterEffects": [ ... ]
    }
  }
}
```

### Recommended Project Structure (new files)
```
app/src/
  main/
    index.ts              # Add project:save, project:load, project:export IPC handlers
    project-io.ts         # NEW: File I/O logic (read/write .calliope files, autosave)
  preload/
    index.ts              # Add saveProject(), loadProject(), exportAudio() convenience methods
  renderer/
    stores/
      project-store.ts    # NEW: dirty flag, current file path, autosave timer, project name
    components/
      export/
        ExportDialog.tsx   # NEW: Modal dialog for export settings
      project/
        UnsavedWarning.tsx # NEW: beforeunload confirmation
engine/
  include/calliope/
    audio_exporter.h      # NEW: Offline bounce + format encoding
  src/
    audio_exporter.cpp    # NEW: Implementation
native/src/
  bridge.cpp              # Add ExportAudio, LoadProjectState bridge functions
  bridge.h                # Declare new exports
```

### Pattern 1: Dual-State Project Serialization
**What:** Project file contains both C++ engine state (from `ProjectState.toJson()`) and Zustand UI state (serialized from renderer stores).
**When to use:** Every save/load operation.
**Why:** The engine tracks instruments, effects, transport. The UI tracks timeline layout, clip arrangement, MIDI notes, mixer volumes/pans. Neither alone is the complete project.
```typescript
// Main process save handler
ipcMain.handle('project:save', async (_event, { filePath, uiState }) => {
  // 1. Get engine state via native bridge
  const engineJson = await native.getProjectState()
  // 2. Combine into project file
  const project = {
    version: 1,
    appVersion: app.getVersion(),
    savedAt: new Date().toISOString(),
    engine: JSON.parse(engineJson),
    ui: uiState
  }
  // 3. Write to disk
  await fs.writeFile(filePath, JSON.stringify(project, null, 2), 'utf-8')
})
```

### Pattern 2: Offline Bounce Rendering (C++)
**What:** Drive AudioProcessorGraph.processBlock() in a loop without audio device, writing output to AudioFormatWriter.
**When to use:** All audio export operations (WAV, FLAC, MP3, stems).
**Why:** Faster-than-realtime rendering. No audio device needed. Reuses existing graph routing.
```cpp
// Pseudocode for offline bounce
bool AudioExporter::bounceToFile(const juce::File& outputFile, int format,
                                  int bitDepth, int bitrate,
                                  ProgressCallback progress)
{
    auto& graph = engine_.getAudioGraph();
    auto sampleRate = graph.getAudioConfig().sampleRate;
    int bufferSize = 512; // offline buffer size

    // Create writer for target format
    std::unique_ptr<juce::AudioFormatWriter> writer;
    // ... create WavAudioFormat/FlacAudioFormat writer

    // Prepare graph for offline rendering
    graph.prepareForOffline(sampleRate, bufferSize);

    juce::AudioBuffer<float> buffer(2, bufferSize);
    juce::MidiBuffer midi;
    int64_t totalSamples = calculateTotalSamples();
    int64_t samplesRendered = 0;

    while (samplesRendered < totalSamples) {
        buffer.clear();
        // Feed MIDI events for this block from clip data
        graph.processBlock(buffer, midi);
        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        samplesRendered += bufferSize;
        progress(static_cast<float>(samplesRendered) / totalSamples);
    }

    return true;
}
```

### Pattern 3: Progress Reporting via TSFN
**What:** Use ThreadSafeFunction with periodic callbacks to report export progress to renderer.
**When to use:** During export operations that take significant time.
```cpp
// In bridge.cpp - export with progress callback
Napi::Value ExportAudio(const Napi::CallbackInfo& info) {
    // ... parse params
    auto progressTsfn = Napi::ThreadSafeFunction::New(env, progressCallback, "ExportProgress", 0, 1);

    std::thread([deferred, tsfn, progressTsfn, /* params */]() {
        auto& engine = calliope::Engine::getInstance();
        // Call exporter with lambda for progress
        bool success = engine.exportAudio(outputPath, format, [&progressTsfn](float pct) {
            progressTsfn.BlockingCall([pct](Napi::Env env, Napi::Function callback) {
                callback.Call({ Napi::Number::New(env, pct) });
            });
        });
        // ... resolve deferred
    }).detach();
}
```

### Pattern 4: Autosave Timer in Main Process
**What:** Main process runs a setInterval that triggers save to `.calliope.autosave`.
**When to use:** Always active when a project has been saved at least once or has unsaved changes.
```typescript
// In main process
let autosaveInterval: ReturnType<typeof setInterval> | null = null
let autosaveIntervalMs = 2 * 60 * 1000 // 2 minutes default

function startAutosave(projectPath: string) {
  stopAutosave()
  autosaveInterval = setInterval(async () => {
    const win = BrowserWindow.getAllWindows()[0]
    if (!win) return
    // Request UI state from renderer
    const uiState = await win.webContents.executeJavaScript(
      'window.__getProjectUIState()'
    )
    // Save to autosave path
    await saveProject(projectPath + '.autosave', uiState)
  }, autosaveIntervalMs)
}
```

### Anti-Patterns to Avoid
- **Saving only engine state or only UI state:** Project file must contain BOTH. Omitting either causes data loss on reload.
- **Synchronous file writes in main process:** Use fs.promises (async) to avoid blocking the event loop.
- **Real-time audio device for export:** Export MUST use offline rendering. Never play through speakers during export.
- **Rendering stems by muting other tracks:** Instead, render each track chain individually. Muting changes engine state and could leave it dirty.
- **Blocking the UI thread during export:** Export runs on a background thread in C++ with progress callbacks.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| WAV file writing | Custom WAV header/data writer | JUCE `WavAudioFormat` + `AudioFormatWriter` | WAV format has nuances (RIFF chunks, padding, RF64 for >4GB). JUCE handles all edge cases. |
| FLAC encoding | Custom FLAC encoder | JUCE `FlacAudioFormat` | FLAC encoding is complex (LPC prediction, rice coding). JUCE bundles libFLAC. |
| MP3 encoding | Custom MP3 bitstream writer | LAME library `lame_encode_buffer_interleaved()` | MP3 encoding is patent-encumbered legacy math. LAME is the definitive encoder. |
| File dialogs | Custom file picker UI | Electron `dialog.showSaveDialog()` / `dialog.showOpenDialog()` | Native OS dialogs handle permissions, recent files, bookmarks correctly. |
| JSON serialization (C++) | Hand-rolled JSON builder | JUCE `juce::JSON` + `juce::DynamicObject` | Already used by ProjectState. Consistent with existing codebase. |

**Key insight:** Every component of save/load/export has a well-tested library solution. The implementation work is orchestration (coordinating the pieces), not algorithm.

## Common Pitfalls

### Pitfall 1: Incomplete State Capture
**What goes wrong:** Loading a project and finding tracks have no clips, or effects are missing, or mixer levels are wrong.
**Why it happens:** ProjectState only captures engine state (instruments, effects, transport). UI state (tracks, clips, MIDI notes, mixer volumes/pans) lives in Zustand stores and is NOT captured by `snapshotFromEngine()`.
**How to avoid:** The save handler MUST request UI state from the renderer process and merge it with engine state into a single JSON file. On load, both must be restored.
**Warning signs:** After load, the timeline shows no tracks or clips even though engine parameters sound correct.

### Pitfall 2: LAME Linking on Multiple Platforms
**What goes wrong:** Build works on macOS but fails on Windows/Linux, or LAME not found.
**Why it happens:** LAME install paths differ across platforms. Homebrew puts it in `/opt/homebrew/` (ARM Mac) or `/usr/local/` (Intel Mac). Linux uses system paths. Windows needs manual build or vcpkg.
**How to avoid:** Use CMake `find_library(LAME_LIB mp3lame)` + `find_path(LAME_INCLUDE lame/lame.h)` with fallback paths. Make MP3 export optional at build time with a CMake option.
**Warning signs:** CMake configure step fails with "Could NOT find mp3lame".

### Pitfall 3: Offline Rendering Graph State
**What goes wrong:** Export produces silence or clicks/glitches at the start.
**Why it happens:** AudioProcessorGraph uses async initialization (`handleAsyncUpdate()`). In offline mode, processBlock may be called before initialization completes. Additionally, the graph may still be connected to an audio device.
**How to avoid:** Before offline rendering: (1) ensure graph connections are fully initialized synchronously, (2) call `prepareToPlay()` with offline sample rate and buffer size on all processors, (3) do NOT disconnect from the audio device (render in parallel, or stop playback first).
**Warning signs:** First few buffers of export contain silence or corrupted audio.

### Pitfall 4: Export Blocking Audio Playback
**What goes wrong:** While exporting, the user cannot play audio or the app freezes.
**Why it happens:** If export uses the same AudioProcessorGraph instance that handles real-time playback, they conflict.
**How to avoid:** Either (1) stop playback during export and use the graph for offline rendering, or (2) create a separate graph instance for export. Option 1 is simpler and sufficient since export produces a progress dialog anyway.
**Warning signs:** App becomes unresponsive during export, or audio glitches occur.

### Pitfall 5: Set Serialization in JSON
**What goes wrong:** Zustand stores use `Set<string>` for selectedClipIds and selectedNoteIds. JSON.stringify ignores Set, producing `{}`.
**Why it happens:** JavaScript `Set` is not JSON-serializable by default.
**How to avoid:** Convert Sets to arrays before serialization: `Array.from(selectedClipIds)`. Convert back on load: `new Set(arr)`. Note: selectedClipIds and selectedNoteIds are UI selection state and may not need to be saved at all.
**Warning signs:** After load, selections are empty.

### Pitfall 6: Float32Array Loss in JSON
**What goes wrong:** Audio clip `peakData` (Float32Array) is lost during save/load.
**Why it happens:** JSON.stringify converts Float32Array to a regular object `{"0": 0.5, "1": 0.3, ...}` which does not reconstruct as Float32Array.
**How to avoid:** Either (1) convert to base64 string for storage and decode on load, or (2) regenerate peak data from audio files on load (preferred -- keeps project files smaller).
**Warning signs:** Waveform display is blank after loading a project.

### Pitfall 7: Unsaved Changes Dialog Race Condition
**What goes wrong:** Window closes before the "unsaved changes" dialog can prevent it.
**Why it happens:** Using `beforeunload` in the renderer is unreliable for async operations. The Electron `close` event fires and the window may close before the dialog resolves.
**How to avoid:** Use `mainWindow.on('close', (event) => { event.preventDefault(); /* show dialog */ })` in the main process, not `beforeunload` in the renderer.
**Warning signs:** Window closes without warning when there are unsaved changes.

## Code Examples

### JUCE WAV/FLAC Export via AudioFormatWriter
```cpp
// Source: JUCE juce_audio_formats documentation
bool writeToWav(const juce::File& file, juce::AudioBuffer<float>& buffer,
                double sampleRate, int bitDepth)
{
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            new juce::FileOutputStream(file),
            sampleRate,
            static_cast<unsigned int>(buffer.getNumChannels()),
            bitDepth,       // 16 or 24
            {},             // metadata
            0               // quality
        )
    );
    if (!writer) return false;
    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}
```

### LAME MP3 Encoding
```cpp
// Source: LAME API documentation (lame.h)
#include <lame/lame.h>

bool encodeToMp3(const juce::File& wavFile, const juce::File& mp3File, int bitrate)
{
    // Read WAV
    juce::WavAudioFormat wavFormat;
    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        wavFormat.createReaderFor(new juce::FileInputStream(wavFile), true));
    if (!reader) return false;

    lame_t lame = lame_init();
    lame_set_in_samplerate(lame, static_cast<int>(reader->sampleRate));
    lame_set_num_channels(lame, static_cast<int>(reader->numChannels));
    lame_set_brate(lame, bitrate);  // e.g., 128, 192, 256, 320
    lame_set_quality(lame, 2);      // 2 = near-best quality
    lame_init_params(lame);

    juce::FileOutputStream mp3Stream(mp3File);
    // ... encode in chunks using lame_encode_buffer_interleaved_ieee_float()
    // ... flush with lame_encode_flush()

    lame_close(lame);
    return true;
}
```

### Electron File Dialog
```typescript
// Source: Electron dialog API docs
import { dialog, BrowserWindow } from 'electron'

async function showSaveDialog(win: BrowserWindow): Promise<string | undefined> {
  const result = await dialog.showSaveDialog(win, {
    title: 'Save Project',
    defaultPath: 'Untitled.calliope',
    filters: [
      { name: 'Calliope Project', extensions: ['calliope'] }
    ]
  })
  return result.canceled ? undefined : result.filePath
}

async function showExportDialog(win: BrowserWindow): Promise<string | undefined> {
  const result = await dialog.showSaveDialog(win, {
    title: 'Export Audio',
    filters: [
      { name: 'WAV Audio', extensions: ['wav'] },
      { name: 'MP3 Audio', extensions: ['mp3'] },
      { name: 'FLAC Audio', extensions: ['flac'] }
    ]
  })
  return result.canceled ? undefined : result.filePath
}
```

### Window Close with Unsaved Changes (Main Process)
```typescript
// Source: Electron BrowserWindow close event
mainWindow.on('close', (event) => {
  if (projectDirty) {
    event.preventDefault()
    dialog.showMessageBox(mainWindow!, {
      type: 'warning',
      buttons: ['Save', "Don't Save", 'Cancel'],
      defaultId: 0,
      cancelId: 2,
      title: 'Unsaved Changes',
      message: 'Do you want to save changes before closing?'
    }).then(({ response }) => {
      if (response === 0) { /* save then close */ }
      else if (response === 1) { mainWindow?.destroy() }
      // response === 2: do nothing (cancel)
    })
  }
})
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Electron remote module for dialogs | dialog API in main process via IPC | Electron 14+ (2021) | Must use main process for all dialog calls; never use @electron/remote |
| JUCE Projucer for builds | CMake-based builds | JUCE 6+ (2020) | CMake find_library for LAME integration |
| Synchronous file I/O (Node) | fs.promises async API | Node 10+ (2018) | All file operations must be async in main process |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | C++ build | Yes | 4.3.0 | -- |
| LAME (libmp3lame) | MP3 export (PROJ-05) | No (not installed) | -- | `brew install lame` required; make MP3 export optional at build time |
| JUCE juce_audio_formats | WAV/FLAC export | Yes | 8.0.12 | -- |
| Electron dialog API | File pickers | Yes | 35.7.5 | -- |
| Node.js fs module | File I/O | Yes | Built-in | -- |

**Missing dependencies with no fallback:**
- None (all have install paths or fallbacks)

**Missing dependencies with fallback:**
- LAME: Not installed. Install with `brew install lame`. MP3 export should be gated behind CMake `find_library()` so the project builds without it (WAV/FLAC still work).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 3.x (TypeScript) + Catch2 (C++) |
| Config file | `vitest.config.ts` (root) + `engine/tests/CMakeLists.txt` |
| Quick run command | `pnpm test` |
| Full suite command | `pnpm test:all` + `cmake --build build --target calliope_tests && ./build/engine/tests/calliope_tests` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROJ-01 | Project save/load roundtrip with full state | integration | `pnpm vitest run test/project-io.test.ts -t "save and load"` | Wave 0 |
| PROJ-02 | Autosave triggers at interval | unit | `pnpm vitest run test/project-store.test.ts -t "autosave"` | Wave 0 |
| PROJ-04 | WAV export produces valid file | integration | C++ test: `calliope_tests "[audio_exporter][wav]"` | Wave 0 |
| PROJ-05 | MP3 export produces valid file | integration | C++ test: `calliope_tests "[audio_exporter][mp3]"` | Wave 0 |
| PROJ-06 | FLAC export produces valid file | integration | C++ test: `calliope_tests "[audio_exporter][flac]"` | Wave 0 |
| PROJ-07 | Stem export produces per-track WAV files | integration | C++ test: `calliope_tests "[audio_exporter][stems]"` | Wave 0 |

### Sampling Rate
- **Per task commit:** `pnpm test`
- **Per wave merge:** `pnpm test:all` + C++ test suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `test/project-io.test.ts` -- covers PROJ-01 (save/load roundtrip, state completeness)
- [ ] `test/project-store.test.ts` -- covers PROJ-02 (dirty flag, autosave timer mock)
- [ ] `engine/tests/test_audio_exporter.cpp` -- covers PROJ-04, PROJ-05, PROJ-06, PROJ-07 (WAV/MP3/FLAC/stems)
- [ ] LAME install: `brew install lame` -- required for MP3 test

## Open Questions

1. **Offline rendering approach: reuse graph vs. create new graph**
   - What we know: The existing AudioProcessorGraph is connected to the audio device. JUCE forum posts indicate offline rendering should call processBlock in a loop.
   - What's unclear: Whether to disconnect from audio device during export (simpler, blocks playback) or create a separate graph (complex, allows concurrent playback).
   - Recommendation: Stop playback and disconnect from audio device during export. Export is a blocking operation with a progress dialog -- users expect not to interact with the DAW during export. This avoids the complexity of duplicating the graph.

2. **MIDI event scheduling during offline bounce**
   - What we know: The offline bounce loop needs to feed MIDI events from clip data into the graph at the right sample positions. Currently, clips and MIDI notes live in Zustand stores (renderer), not in the C++ engine.
   - What's unclear: How to get MIDI event data into the C++ offline renderer.
   - Recommendation: Pass MIDI note data as JSON from renderer to main process to C++ before starting the bounce. The exporter parses this and schedules MIDI events into the MidiBuffer for each processBlock call based on beat position.

3. **Sample rate for export vs. project sample rate**
   - What we know: Project runs at 44.1kHz or 48kHz (configurable). Export should match project rate.
   - What's unclear: Whether users should be able to choose a different export sample rate.
   - Recommendation: For v1, export at the project's sample rate. Add sample rate conversion (libsamplerate) in a future version if needed.

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.12 API -- AudioFormatWriter, WavAudioFormat, FlacAudioFormat (linked in engine CMakeLists.txt)
- [Electron dialog API](https://www.electronjs.org/docs/latest/api/dialog) -- showSaveDialog, showOpenDialog
- [Electron BrowserWindow close event](https://www.electronjs.org/docs/latest/api/browser-window#event-close) -- preventing close for unsaved changes
- Existing codebase: ProjectState.toJson()/fromJson(), snapshotFromEngine(), bridge.cpp patterns

### Secondary (MEDIUM confidence)
- [JUCE Forum: Offline Rendering AudioProcessorGraph](https://forum.juce.com/t/offline-rendering-audioprocessorgraph-solved/32370) -- async init pitfall
- [JUCE Forum: AudioProcessorGraph for file rendering](https://forum.juce.com/t/audioprocessorgraph-used-for-rendering-files/49643) -- offline bounce pattern
- [LAME API (lame.h)](https://lame.sourceforge.io/) -- lame_encode_buffer_interleaved, lame_init, lame_set_brate

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - all libraries already in project except LAME (well-documented, brew-installable)
- Architecture: HIGH - extends existing IPC + native bridge patterns; offline rendering is standard JUCE
- Pitfalls: HIGH - identified from codebase analysis (dual-state problem, Set serialization, LAME linking)

**Research date:** 2026-03-29
**Valid until:** 2026-04-28 (stable domain, no fast-moving dependencies)
