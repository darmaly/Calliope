# Requirements: LuneyTunes

**Defined:** 2026-03-27
**Core Value:** A person with a musical vision but no DAW experience can create professional-sounding music with AI assistance.

## v1 Requirements

Requirements for initial release. AI-ready DAW foundation with professional sound quality.

### Audio Engine

- [x] **ENG-01**: Audio engine processes multi-track audio in real-time at 44.1kHz/48kHz, 32-bit float internal
- [x] **ENG-02**: Audio engine supports configurable buffer sizes (128-2048 samples) with stable playback
- [ ] **ENG-03**: Audio routing supports per-track insert effect chains with serial processing
- [x] **ENG-04**: Audio routing supports master bus with insert chain for final output processing
- [x] **ENG-05**: Transport controls: play, stop, pause, loop region, with BPM and time signature control
- [x] **ENG-06**: Metronome with toggleable click track and adjustable volume

### Instruments

- [x] **INST-01**: Subtractive/wavetable polysynth with oscillators, filter, envelopes, and LFO
- [x] **INST-02**: Bass synthesizer optimized for low-frequency sounds with sub-oscillator
- [x] **INST-03**: Sample-based drum machine / sampler that loads WAV/MP3 samples and maps to pads/keys
- [ ] **INST-04**: Each instrument exposes all parameters via the command dispatcher interface

### Effects

- [ ] **FX-01**: Parametric EQ with at least 4 bands (low shelf, 2 parametric, high shelf)
- [ ] **FX-02**: Dynamic compressor with threshold, ratio, attack, release, makeup gain
- [ ] **FX-03**: Algorithmic reverb with room size, damping, wet/dry, pre-delay
- [ ] **FX-04**: Tempo-synced delay with feedback, wet/dry, and ping-pong option
- [ ] **FX-05**: Brick-wall limiter on master bus for final output loudness control
- [ ] **FX-06**: Each effect exposes all parameters via the command dispatcher interface

### Timeline & Arrangement

- [ ] **TL-01**: Multi-track horizontal timeline with zoomable view and playhead
- [ ] **TL-02**: MIDI clips can be created, moved, resized, copied, and deleted on tracks
- [ ] **TL-03**: Audio clips display waveforms and can be placed on tracks
- [ ] **TL-04**: Tracks can be added, removed, reordered, renamed, and color-coded
- [ ] **TL-05**: Snap-to-grid with configurable grid resolution (1/4, 1/8, 1/16, 1/32 notes)
- [ ] **TL-06**: Loop region selection for repeated playback of a section

### Piano Roll

- [ ] **PR-01**: Piano roll editor displays MIDI notes on a pitch/time grid with velocity
- [ ] **PR-02**: Notes can be drawn, selected, moved, resized, copied, and deleted
- [ ] **PR-03**: Velocity editing per note via velocity lane or note color
- [ ] **PR-04**: Quantize function snaps selected notes to grid resolution
- [ ] **PR-05**: Piano roll supports scroll, zoom, and keyboard reference on left edge

### Mixer

- [ ] **MIX-01**: Per-track channel strip with volume fader, pan knob, mute, and solo
- [ ] **MIX-02**: Master channel strip with volume fader and insert effect chain
- [ ] **MIX-03**: Per-track insert effect chain with add, remove, reorder, and bypass per effect
- [ ] **MIX-04**: Visual level meters on each channel strip showing real-time signal level

### Project Management

- [ ] **PROJ-01**: Project save to file and load from file with full state restoration
- [ ] **PROJ-02**: Autosave at configurable interval (default every 2 minutes)
- [x] **PROJ-03**: Undo/redo with deep history stack (minimum 100 operations)
- [ ] **PROJ-04**: Export final mix to WAV (uncompressed, 16/24-bit)
- [ ] **PROJ-05**: Export final mix to MP3 (configurable bitrate)
- [ ] **PROJ-06**: Export final mix to FLAC (lossless)
- [ ] **PROJ-07**: Export individual track stems as separate WAV files

### Architecture (AI-Ready Foundation)

- [x] **ARCH-01**: Single command dispatcher — all operations (UI, future AI) flow through one interface
- [x] **ARCH-02**: DAW state serializable to JSON — full project state readable by external consumers
- [x] **ARCH-03**: Native bridge (node-addon-api) connects C++ audio engine to Electron/Node.js layer
- [x] **ARCH-04**: Lock-free FIFO communication between audio thread and UI/command thread
- [x] **ARCH-05**: Every instrument and effect parameter addressable by ID via command dispatcher
- [x] **ARCH-06**: Command dispatcher emits events for all state changes (enables future AI state tracking)

### User Interface

- [x] **UI-01**: Electron application shell with React-based UI
- [ ] **UI-02**: Main layout with timeline, piano roll, mixer, and instrument/effect panels
- [ ] **UI-03**: Waveform rendering for audio clips using PixiJS/WebGL
- [ ] **UI-04**: Responsive transport bar with BPM, time signature, play/stop/record controls
- [ ] **UI-05**: Keyboard shortcuts for transport, undo/redo, save, and common operations

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### AI Integration

- **AI-01**: Natural language chat interface with persistent conversation history
- **AI-02**: BYOK API key management for Claude, GPT, and other providers
- **AI-03**: AI generates MIDI from text descriptions ("write a jazzy chord progression in Cm")
- **AI-04**: AI selects and loads instruments/presets from descriptions ("warm pad", "punchy bass")
- **AI-05**: AI applies and configures effects from descriptions ("more reverb", "tighter low end")
- **AI-06**: AI adjusts mixer levels from descriptions ("make the drums louder")
- **AI-07**: AI explains actions to user ("I added a compressor to tighten the bass")
- **AI-08**: Full DAW state visibility — AI reads structured project state
- **AI-09**: End-to-end workflow: describe a song, AI produces a finished track
- **AI-10**: AI-driven arrangement (intro, verse, chorus, bridge, outro)
- **AI-11**: AI-driven mixing chain with professional processing
- **AI-12**: AI-driven mastering with loudness targeting

### Advanced Features

- **ADV-01**: Automation lanes for parameter changes over time
- **ADV-02**: Send/return effect routing
- **ADV-03**: Genre-aware production templates
- **ADV-04**: Multiple AI "takes" / alternatives
- **ADV-05**: Context-aware proactive AI suggestions
- **ADV-06**: Musical theory awareness in AI (keys, scales, chord functions)
- **ADV-07**: Guided onboarding via AI conversation
- **ADV-08**: Visual feedback of AI actions (notes appearing, faders moving)

### Production Enhancement

- **PROD-01**: AI stem separation (Demucs-based)
- **PROD-02**: Audio-to-MIDI transcription
- **PROD-03**: Reference track matching
- **PROD-04**: Natural language sound design (text to synth preset)
- **PROD-05**: Smart quantization with humanization

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| VST/AU plugin hosting | Enormous complexity; AI can't control arbitrary plugins; built-in instruments sufficient |
| Audio recording from microphone | Hardware dependency, latency management; import audio files instead |
| Collaboration / cloud features | Multi-user sync is a product in itself; single-user local app |
| Mobile version | DAW UI compromised on mobile; desktop-first via Electron |
| Notation / score view | Target users don't read sheet music; piano roll sufficient |
| Video sync / film scoring | Different workflow and user base; music-only |
| Subscription-gated AI hosting | BYOK keeps infra simple and costs on user |
| "Push button, get song" with no interaction | Competes with Suno/Udio; no user agency; not a DAW |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| ARCH-03 | Phase 1 | Complete |
| UI-01 | Phase 1 | Complete |
| ENG-01 | Phase 2 | Complete |
| ENG-02 | Phase 2 | Complete |
| ENG-04 | Phase 2 | Complete |
| ENG-05 | Phase 2 | Complete |
| ENG-06 | Phase 2 | Complete |
| ARCH-04 | Phase 2 | Complete |
| ARCH-01 | Phase 3 | Complete |
| ARCH-02 | Phase 3 | Complete |
| ARCH-05 | Phase 3 | Complete |
| ARCH-06 | Phase 3 | Complete |
| PROJ-03 | Phase 3 | Complete |
| INST-01 | Phase 4 | Complete |
| INST-02 | Phase 4 | Complete |
| INST-03 | Phase 4 | Complete |
| INST-04 | Phase 4 | Pending |
| FX-01 | Phase 5 | Pending |
| FX-02 | Phase 5 | Pending |
| FX-03 | Phase 5 | Pending |
| FX-04 | Phase 5 | Pending |
| FX-05 | Phase 5 | Pending |
| FX-06 | Phase 5 | Pending |
| ENG-03 | Phase 5 | Pending |
| TL-01 | Phase 6 | Pending |
| TL-02 | Phase 6 | Pending |
| TL-03 | Phase 6 | Pending |
| TL-04 | Phase 6 | Pending |
| TL-05 | Phase 6 | Pending |
| TL-06 | Phase 6 | Pending |
| PR-01 | Phase 7 | Pending |
| PR-02 | Phase 7 | Pending |
| PR-03 | Phase 7 | Pending |
| PR-04 | Phase 7 | Pending |
| PR-05 | Phase 7 | Pending |
| MIX-01 | Phase 8 | Pending |
| MIX-02 | Phase 8 | Pending |
| MIX-03 | Phase 8 | Pending |
| MIX-04 | Phase 8 | Pending |
| PROJ-01 | Phase 9 | Pending |
| PROJ-02 | Phase 9 | Pending |
| PROJ-04 | Phase 9 | Pending |
| PROJ-05 | Phase 9 | Pending |
| PROJ-06 | Phase 9 | Pending |
| PROJ-07 | Phase 9 | Pending |
| UI-02 | Phase 10 | Pending |
| UI-03 | Phase 10 | Pending |
| UI-04 | Phase 10 | Pending |
| UI-05 | Phase 10 | Pending |

**Coverage:**
- v1 requirements: 49 total
- Mapped to phases: 49
- Unmapped: 0

---
*Requirements defined: 2026-03-27*
*Last updated: 2026-03-27 after roadmap creation*
