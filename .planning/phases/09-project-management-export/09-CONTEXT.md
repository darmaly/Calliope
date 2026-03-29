# Phase 9: Project Management & Export - Context

**Gathered:** 2026-03-29
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can save, load, and export their Calliope projects. Save/load preserves full DAW state (tracks, clips, MIDI, mixer, effects, transport). Export produces final audio in WAV, MP3, and FLAC formats, plus individual track stems.

</domain>

<decisions>
## Implementation Decisions

### Project File Format & Save/Load
- Project file format is JSON with `.calliope` extension — human-readable, consistent with ProjectState JSON serialization from Phase 3
- Save/load via File > Save / File > Open menu items plus Ctrl+S / Ctrl+O keyboard shortcuts
- Autosave writes to same directory as project file with `.calliope.autosave` suffix
- Full DAW state restoration: tracks, clips, MIDI notes, mixer settings (volume/pan/effects), transport position — everything in ProjectState

### Audio Export
- Export workflow uses a modal dialog with format selection, quality options, and file path picker
- Export processing is offline bounce through C++ engine (faster than real-time) with progress bar
- MP3 encoding via LAME library in C++ engine (LGPL-compatible, per CLAUDE.md stack spec)
- Stem export produces one WAV per track in a `stems/` subfolder next to the export file

### Autosave & Error Handling
- Autosave interval defaults to 2 minutes, configurable in settings
- Unsaved changes shown via title bar asterisk (*) and "Unsaved changes" warning on window close
- Export errors displayed as toast notification with error message and retry button

### Claude's Discretion
- Specific modal dialog layout and styling
- Progress bar animation style
- Settings UI for autosave interval configuration
- File picker default directory handling

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ProjectState` (C++) — JSON serialization via `juce::DynamicObject` + `juce::JSON`, already handles tracks, clips, instruments, effects
- `CommandDispatcher` — all state changes go through commands, can serialize/deserialize command history
- `MasterBusProcessor` — offline rendering capability (processBlock without real-time constraint)
- `AudioGraph` — track routing for per-track stem rendering
- `window.calliope.dispatchCommand()` — existing IPC for all engine operations
- JUCE `juce_audio_formats` — WAV, AIFF, FLAC read/write built in
- LAME encoder referenced in CLAUDE.md stack spec for MP3 export

### Established Patterns
- Electron `dialog.showSaveDialog()` / `dialog.showOpenDialog()` for native file pickers
- IPC invoke pattern for async operations with Promise-based responses
- Zustand store for UI state (project-store for save/load/dirty tracking)
- Toast/notification pattern can follow portal-based popover approach from Phases 6-8

### Integration Points
- Main process: file I/O handlers (`ipcMain.handle('project:save')`, `project:load`, `project:export`)
- Preload: convenience methods (`window.calliope.saveProject()`, `loadProject()`, `exportAudio()`)
- C++ engine: serialize/deserialize ProjectState, offline bounce rendering
- Menu bar: File menu with Save/Open/Export items + accelerators
- Title bar: dirty state indicator (asterisk)
- Window close: beforeunload handler for unsaved changes prompt

</code_context>

<specifics>
## Specific Ideas

- Project name is "Calliope" — file extension is `.calliope`, not `.ltp`
- User wants all documentation references updated from "LuneyTunes" to "Calliope"

</specifics>

<deferred>
## Deferred Ideas

- Rename "LuneyTunes" to "Calliope" across all project documentation (tracked separately, not part of Phase 9 scope)

</deferred>
