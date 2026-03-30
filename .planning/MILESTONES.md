# Milestones

## v1.0 Calliope DAW MVP (Shipped: 2026-03-30)

**Phases completed:** 11 phases, 34 plans, 64 tasks

**Key accomplishments:**

- 1. [Rule 3 - Blocking] pnpm build script approval required
- Transport state machine (AudioPlayHead) with atomic BPM/position/loop state and SPSC lock-free queue via AbstractFifo, tested with 22 Catch2 unit tests
- AudioProcessorGraph mixing architecture with metronome -> master -> output wiring, beat-synced click generation, and Engine singleton owning full audio lifecycle
- 13 ThreadSafeFunction bridge functions wiring Engine lifecycle, transport, metronome, and config APIs from C++ through IPC to window.calliope, verified with audible metronome playback
- CommandDispatcher with 200-transaction undo/redo via juce::UndoManager, ParameterRegistry for string-ID parameter addressing, and ProjectState with JSON round-trip serialization
- Transport, metronome, and master bus command classes with undo support, plus Engine integration with CommandDispatcher and ParameterRegistry owning 6 registered parameters
- Full-stack command dispatch from renderer to C++ engine via bridge with undo/redo, state query, parameter IDs, and event subscription through persistent ThreadSafeFunction
- PolyBLEP-based PolySynth (16-voice, dual osc, LadderFilter, LFO) and BassSynth (4-voice, sub-oscillator) with full Catch2 test coverage
- DrumMachineProcessor with 16-pad GM drum map using JUCE SamplerSound/SamplerVoice for WAV/AIFF/MP3 sample playback triggered via MIDI
- All three instruments wired into AudioGraph as graph nodes with 36 registered parameters, MIDI command dispatch, and full bridge/IPC integration
- Five JUCE DSP effect processors (EQ, compressor, reverb, delay, limiter) and lock-free InsertChain container with double-buffer pattern for real-time safe chain modification
- Per-track and master insert chain routing in AudioGraph with 4 undoable effect commands, dynamic parameter registration, and ProjectState serialization
- Bridge dispatch for 4 effect commands and preload convenience API completing the full renderer-to-audio-engine effects pipeline
- Multi-track timeline layout with PixiJS canvas, pixi-viewport scroll/zoom, grid overlay, CSS playhead, and track headers with mute/solo/arm controls
- PixiJS clip rendering (MIDI + audio waveform), selection box, loop region overlay, and clip operation utilities with 15 passing tests
- Fully interactive timeline arrangement view with clip CRUD via mouse, context menus for track/clip management, keyboard shortcuts, and engine event sync
- Zustand piano-roll store with MidiNote CRUD, note operations (move/resize/quantize/velocity-scale), piano helpers, and triplet grid support
- PixiJS piano roll canvas with 128-key keyboard, pitch/time grid, velocity-alpha notes, collapsible velocity lane, and split panel layout
- Full MIDI note interaction layer with draw, select, move, resize, velocity editing, keyboard shortcuts, context menus, and timeline double-click opening
- Zustand mixer store with per-track volume/pan/levels/effects, full channel strip UI (fader, pan knob, level meter, effect inserts, mute/solo), master strip, and app integration via toolbar toggle
- Real-time RMS/peak level metering from C++ audio thread through atomic reads, native bridge, IPC, to renderer preload API
- Complete mixer panel with 10 channel strip components, Canvas 2D level meters, effect slot management, and App.tsx integration via toolbar toggle
- HTML5 drag-and-drop effect slot reorder and store-driven mixer panel resize closing two verified gaps
- Versioned JSON project serialization with Electron file dialogs, main-process autosave timer, and Zustand dirty tracking
- Export dialog with WAV/MP3/FLAC format selection, progress bar overlay, toast notifications, and MIDI event collection wired to C++ bridge
- Fixed duplicate processBlock bug producing silent exports and registered ExportAudio/ExportStems/LoadProjectState in native bridge with TSFN progress callbacks
- Added 8 missing export API type declarations to CalliopeAPI interface for zero TypeScript compilation errors
- Transport bar with play/stop/record controls, editable BPM, dual-format position display, loop/metronome toggles, and panel navigation
- Focus-aware keyboard shortcut routing with pure dispatch function, panel focus UI indicators, and window title sync
- Restored getMeterLevels/setTrackVolume/setTrackPan IPC path lost in Phase 9 merge, added engine init call and dirty tracking activation on app mount
- ClipScheduler C++ class dispatches MIDI events from clips to instruments during audio callback, with full IPC pipeline from renderer through preload/bridge to engine

---
