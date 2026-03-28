# Roadmap: LuneyTunes

## Overview

LuneyTunes is built bottom-up: first the hybrid build system and native bridge, then the C++ audio engine with real-time DSP, then the command dispatcher that unifies all operations, then instruments and effects that produce sound, then the UI layers (timeline, piano roll, mixer) that let users see and interact with the DAW, then project management and export, and finally full application integration that ties every panel into a cohesive workspace. Each phase delivers a verifiable capability that the next phase depends on.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Build System & App Shell** - Electron app loads a C++ native addon via cmake-js and node-addon-api
- [ ] **Phase 2: Audio Engine Core** - Real-time multi-track audio processing with transport controls and lock-free threading
- [ ] **Phase 3: Command Dispatcher & State** - Single command interface for all DAW operations with undo/redo and serializable state
- [ ] **Phase 4: Instruments** - Built-in synthesizers and sampler that produce sound through the audio engine
- [ ] **Phase 5: Effects Processing** - Built-in effects chain with per-track insert routing
- [ ] **Phase 6: Timeline & Arrangement** - Multi-track timeline view with clip management and arrangement editing
- [ ] **Phase 7: Piano Roll** - MIDI note editor with drawing, editing, velocity, and quantize
- [ ] **Phase 8: Mixer** - Channel strip mixer with volume, pan, mute/solo, effect inserts, and level meters
- [ ] **Phase 9: Project Management & Export** - Save/load, autosave, and export to WAV/MP3/FLAC/stems
- [ ] **Phase 10: Application Integration** - Unified layout, waveform rendering, transport bar, and keyboard shortcuts

## Phase Details

### Phase 1: Build System & App Shell
**Goal**: A working hybrid application where Electron successfully loads and calls into a C++ native addon built with JUCE and cmake-js
**Depends on**: Nothing (first phase)
**Requirements**: ARCH-03, UI-01
**Success Criteria** (what must be TRUE):
  1. Electron application launches and renders a React-based window
  2. C++ native addon (built with cmake-js) loads successfully in the Electron main process
  3. A round-trip call from JavaScript to C++ and back returns a valid result
  4. The build produces a working application on macOS with a single build command
**Plans**: TBD

### Phase 2: Audio Engine Core
**Goal**: The C++ audio engine processes multi-track audio in real-time with stable playback, transport controls, and lock-free thread communication
**Depends on**: Phase 1
**Requirements**: ENG-01, ENG-02, ENG-04, ENG-05, ENG-06, ARCH-04
**Success Criteria** (what must be TRUE):
  1. Audio engine outputs sound at 44.1kHz/48kHz with 32-bit float internal processing
  2. User can play, stop, pause, and loop audio with BPM and time signature control
  3. Buffer size is configurable (128-2048 samples) without audible glitches during playback
  4. Master bus processes audio through its insert chain for final output
  5. Metronome click plays in sync with transport at the set BPM
**Plans**: TBD

### Phase 3: Command Dispatcher & State
**Goal**: All DAW operations flow through a single command dispatcher with full undo/redo and JSON-serializable project state
**Depends on**: Phase 2
**Requirements**: ARCH-01, ARCH-02, ARCH-05, ARCH-06, PROJ-03
**Success Criteria** (what must be TRUE):
  1. Every operation (parameter change, track edit, transport action) executes through the command dispatcher
  2. Full project state is serializable to JSON and readable by external consumers
  3. Every instrument and effect parameter is addressable by ID via the dispatcher
  4. Command dispatcher emits events for all state changes
  5. Undo/redo works across all operations with a minimum 100-operation history stack
**Plans**: TBD

### Phase 4: Instruments
**Goal**: Users can load built-in synthesizers and a sampler that produce sound through the audio engine
**Depends on**: Phase 3
**Requirements**: INST-01, INST-02, INST-03, INST-04
**Success Criteria** (what must be TRUE):
  1. Polysynth produces sound with configurable oscillators, filter, envelopes, and LFO
  2. Bass synth produces low-frequency sounds with sub-oscillator
  3. Drum machine / sampler loads WAV/MP3 samples and maps them to pads/keys
  4. All instrument parameters are controllable via the command dispatcher
**Plans**: TBD

### Phase 5: Effects Processing
**Goal**: Built-in audio effects can be applied to tracks via insert chains with per-track routing
**Depends on**: Phase 4
**Requirements**: FX-01, FX-02, FX-03, FX-04, FX-05, FX-06, ENG-03
**Success Criteria** (what must be TRUE):
  1. Parametric EQ, compressor, reverb, delay, and limiter each process audio correctly
  2. Effects can be inserted, removed, reordered, and bypassed on any track's insert chain
  3. Per-track insert chains process audio in serial before reaching the master bus
  4. Limiter on the master bus controls final output loudness
  5. All effect parameters are controllable via the command dispatcher
**Plans**: TBD

### Phase 6: Timeline & Arrangement
**Goal**: Users can arrange music on a multi-track horizontal timeline with clips, tracks, and grid snapping
**Depends on**: Phase 5
**Requirements**: TL-01, TL-02, TL-03, TL-04, TL-05, TL-06
**Success Criteria** (what must be TRUE):
  1. Multi-track timeline displays with horizontal scrolling, zoom, and a moving playhead
  2. MIDI clips can be created, moved, resized, copied, and deleted on tracks
  3. Audio clips display waveforms and can be placed on tracks
  4. Tracks can be added, removed, reordered, renamed, and color-coded
  5. Snap-to-grid works with configurable resolution (1/4, 1/8, 1/16, 1/32 notes)
**Plans**: TBD
**UI hint**: yes

### Phase 7: Piano Roll
**Goal**: Users can edit MIDI notes on a pitch/time grid with velocity control and quantization
**Depends on**: Phase 6
**Requirements**: PR-01, PR-02, PR-03, PR-04, PR-05
**Success Criteria** (what must be TRUE):
  1. Piano roll displays MIDI notes on a pitch/time grid with velocity indication
  2. Notes can be drawn, selected, moved, resized, copied, and deleted
  3. Per-note velocity is editable via velocity lane or note color
  4. Quantize function snaps selected notes to the current grid resolution
  5. Piano roll supports scroll, zoom, and shows keyboard reference on the left edge
**Plans**: TBD
**UI hint**: yes

### Phase 8: Mixer
**Goal**: Users can mix tracks with volume, pan, mute/solo, effect inserts, and real-time level meters
**Depends on**: Phase 5
**Requirements**: MIX-01, MIX-02, MIX-03, MIX-04
**Success Criteria** (what must be TRUE):
  1. Each track has a channel strip with volume fader, pan knob, mute, and solo
  2. Master channel strip has volume fader and insert effect chain
  3. Per-track insert chain supports add, remove, reorder, and bypass per effect
  4. Real-time level meters display signal level on each channel strip
**Plans**: TBD
**UI hint**: yes

### Phase 9: Project Management & Export
**Goal**: Users can save, load, and export their projects in multiple audio formats
**Depends on**: Phase 5
**Requirements**: PROJ-01, PROJ-02, PROJ-04, PROJ-05, PROJ-06, PROJ-07
**Success Criteria** (what must be TRUE):
  1. Project saves to file and loads from file with full state restoration
  2. Autosave triggers at a configurable interval (default every 2 minutes)
  3. Final mix exports to WAV (16/24-bit), MP3 (configurable bitrate), and FLAC
  4. Individual track stems export as separate WAV files
**Plans**: TBD

### Phase 10: Application Integration
**Goal**: All panels unite into a cohesive DAW workspace with professional navigation, rendering, and shortcuts
**Depends on**: Phase 6, Phase 7, Phase 8, Phase 9
**Requirements**: UI-02, UI-03, UI-04, UI-05
**Success Criteria** (what must be TRUE):
  1. Main layout displays timeline, piano roll, mixer, and instrument/effect panels in a unified view
  2. Audio clip waveforms render via PixiJS/WebGL with smooth performance
  3. Transport bar shows BPM, time signature, and play/stop controls with real-time playhead position
  4. Keyboard shortcuts work for transport, undo/redo, save, and common operations
**Plans**: TBD
**UI hint**: yes

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10
Note: Phases 6, 7, 8, 9 can execute in parallel after Phase 5 (they share the same dependency).
Phase 10 waits for 6, 7, 8, and 9 to complete.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Build System & App Shell | 0/TBD | Not started | - |
| 2. Audio Engine Core | 0/TBD | Not started | - |
| 3. Command Dispatcher & State | 0/TBD | Not started | - |
| 4. Instruments | 0/TBD | Not started | - |
| 5. Effects Processing | 0/TBD | Not started | - |
| 6. Timeline & Arrangement | 0/TBD | Not started | - |
| 7. Piano Roll | 0/TBD | Not started | - |
| 8. Mixer | 0/TBD | Not started | - |
| 9. Project Management & Export | 0/TBD | Not started | - |
| 10. Application Integration | 0/TBD | Not started | - |
