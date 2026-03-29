---
phase: 09-project-management-export
plan: 02
subsystem: audio-export
tags: [export, wav, mp3, flac, lame, offline-bounce, stems, bridge]
dependency_graph:
  requires: [engine, audio-graph, transport, juce-audio-formats]
  provides: [audio-exporter, export-bridge, export-ipc]
  affects: [native-bridge, preload, main-process]
tech_stack:
  added: [LAME 3.100]
  patterns: [offline-bounce, TSFN-progress, format-encoding]
key_files:
  created:
    - engine/include/calliope/audio_exporter.h
    - engine/src/audio_exporter.cpp
    - engine/tests/test_audio_exporter.cpp
  modified:
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt
    - engine/include/calliope/engine.h
    - engine/src/engine.cpp
    - native/src/bridge.h
    - native/src/bridge.cpp
    - native/src/addon.cpp
    - app/src/main/native-bridge.ts
    - app/src/main/index.ts
    - app/src/preload/index.ts
decisions:
  - "LAME linked via CMake find_library with optional #ifdef guard for MP3 support"
  - "Offline bounce stops transport and drives processBlock in tight loop"
  - "MIDI events passed as JSON with camelCase field names matching JS contract"
  - "Stem export filters MIDI events by trackId for per-instrument rendering"
metrics:
  duration: 1114s
  completed: "2026-03-29T22:43:18Z"
---

# Phase 9 Plan 02: Audio Export Engine Summary

C++ AudioExporter with LAME MP3 encoding, JUCE WAV/FLAC writing, offline bounce loop, stem export, and native bridge with progress TSFN callbacks.

## What Was Built

### Task 1: AudioExporter C++ class (3851176)
- `AudioExporter` class with offline bounce loop that drives `AudioProcessorGraph.processBlock()` in a tight loop for faster-than-realtime rendering
- MIDI event scheduling during offline bounce with proper note-on/note-off timing based on beat positions
- `writeWav()` using JUCE `WavAudioFormat` for 16-bit and 24-bit PCM output
- `writeFlac()` using JUCE `FlacAudioFormat` for lossless compression
- `writeMp3()` using LAME library (`lame_encode_buffer_interleaved_ieee_float`) with configurable bitrate, guarded by `#ifdef CALLIOPE_HAS_LAME`
- `parseMidiEventsJson()` static method parsing exact camelCase field names: `beatPosition`, `noteNumber`, `velocity`, `durationBeats`, `trackId`
- `exportStems()` renders each track individually by filtering MIDI events per trackId
- CMake `find_library(LAME_LIB mp3lame)` with optional compile definition
- `Engine::getAudioExporter()` accessor with lazy initialization in `Engine::initialise()`
- Catch2 tests: JSON parsing (4 cases) and WAV export integration test

### Task 2: Native bridge functions (9a0dba2)
- `ExportAudio` bridge: accepts outputPath, format, mp3Bitrate, totalBeats, midiEventsJson, optional progress callback via TSFN
- `ExportStems` bridge: accepts outputDir, totalBeats, midiEventsJson, optional progress callback
- `LoadProjectState` bridge: accepts JSON string, restores C++ engine state
- Registered in addon.cpp as `exportAudio`, `exportStems`, `loadProjectState`
- TypeScript types in `native-bridge.ts` for all three functions
- IPC handlers: `project:export`, `project:exportStems`, `project:loadState`
- Preload API: `exportAudio()`, `exportStems()`, `loadProjectState()`, `onExportProgress()`
- Progress forwarded from C++ background thread to renderer via `webContents.send('project:exportProgress')`

## Decisions Made

1. **LAME via CMake find_library**: Optional at build time with `CALLIOPE_HAS_LAME` compile definition. MP3 export gracefully disabled if LAME not installed.
2. **Offline bounce approach**: Stop transport, drive `processBlock()` in loop. No separate graph instance -- reuses existing graph. Simpler and sufficient since export shows a progress dialog.
3. **MIDI event routing**: trackId mapped to MIDI channels (polySynth=1, bassSynth=2, drumMachine=10) for instrument routing during offline bounce.
4. **Stem export via event filtering**: Rather than muting/soloing tracks (which changes engine state), stem export filters MIDI events by trackId so only the target instrument receives notes.

## Deviations from Plan

None -- plan executed exactly as written.

## Known Stubs

None. All export formats are fully implemented with real encoding (WAV via JUCE, FLAC via JUCE, MP3 via LAME).

## Verification

- CMake configures with "LAME found: /usr/local/lib/libmp3lame.dylib"
- `calliope_engine` library builds without errors
- `calliope_engine_tests "[export]"` passes all 5 test cases (539 assertions)
- `calliope_addon` native module builds and links successfully

## Self-Check: PASSED
