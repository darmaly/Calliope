# Architecture Patterns

**Domain:** AI-powered Digital Audio Workstation (hybrid C++/Electron)
**Researched:** 2026-03-27

## Recommended Architecture

LuneyTunes is a three-process architecture: a C++ real-time audio engine, an Electron main process (Node.js) acting as the coordination hub, and Electron renderer processes (React) providing the UI. An AI command layer sits in the Node.js process, translating natural language into structured DAW operations.

```
+------------------------------------------------------------------+
|                        Electron App                               |
|                                                                   |
|  +-----------------------+     +------------------------------+   |
|  |   Renderer (React)    |     |   Main Process (Node.js)     |   |
|  |                       |     |                               |   |
|  |  Piano Roll           |     |  Project State Manager        |   |
|  |  Timeline/Arrangement |<--->|  Command Dispatcher           |   |
|  |  Mixer                | IPC |  AI Integration Layer         |   |
|  |  Chat Interface       |     |  File I/O                     |   |
|  |  Waveform Display     |     |  Native Addon Bridge          |   |
|  +-----------------------+     +-----+-------------------------+   |
|                                      |                             |
|                                      | N-API (node-addon-api)     |
|                                      |                             |
|  +-----------------------------------v-------------------------+   |
|  |              C++ Audio Engine (JUCE)                        |   |
|  |                                                             |   |
|  |  Audio Thread (RT)          Message Thread                  |   |
|  |  +-------------------+      +-------------------------+    |   |
|  |  | DSP Graph         |      | Parameter Store         |    |   |
|  |  | Synth Voices      |<---->| MIDI Sequencer          |    |   |
|  |  | Effects Chain     | FIFO | Transport Control        |    |   |
|  |  | Audio I/O         |      | Meter/Analysis Output    |    |   |
|  |  +-------------------+      +-------------------------+    |   |
|  +-------------------------------------------------------------+   |
+------------------------------------------------------------------+
```

### Component Boundaries

| Component | Responsibility | Communicates With | Thread Safety |
|-----------|---------------|-------------------|---------------|
| **C++ Audio Thread** | Real-time DSP processing, synthesis, effects, audio I/O callback | C++ Message Thread via lock-free FIFO | Hard real-time; no allocations, no locks, no syscalls |
| **C++ Message Thread** | Parameter changes, MIDI sequencing, transport, meter data export | Audio Thread via FIFO; Node.js via N-API | Soft real-time; can allocate but should be fast |
| **Node.js Main Process** | Project state, command dispatch, AI integration, file I/O, native addon bridge | C++ Message Thread via N-API; Renderer via Electron IPC | Event-loop driven; async operations preferred |
| **React Renderer** | UI rendering (piano roll, timeline, mixer, chat), user input capture | Main Process via Electron IPC (contextBridge) | UI thread; requestAnimationFrame for visual updates |
| **AI Integration Layer** | API key management, prompt construction, response parsing, command translation | Main Process command dispatcher; external AI APIs via HTTP | Async; never blocks audio or UI |

### Data Flow

**User action (e.g., adjust volume fader):**
```
React UI -> Electron IPC -> Main Process -> Command Dispatcher
  -> N-API call -> C++ Message Thread -> lock-free FIFO -> Audio Thread
```

**AI action (e.g., "add reverb to track 2"):**
```
Chat UI -> Electron IPC -> Main Process -> AI Layer -> External API
  -> AI response parsed -> Command Dispatcher
  -> N-API call -> C++ Message Thread -> lock-free FIFO -> Audio Thread
```

**Audio engine feedback (e.g., meter levels, playback position):**
```
Audio Thread -> lock-free FIFO -> C++ Message Thread
  -> N-API callback/polling -> Main Process -> Electron IPC -> React UI
```

**State synchronization (e.g., piano roll showing current notes):**
```
Main Process holds authoritative project state (JSON-serializable)
  -> Pushes deltas to Renderer via IPC on change
  -> Pushes relevant changes to C++ engine via N-API
```

---

## Component Deep Dives

### 1. C++ Audio Engine (JUCE)

**Two-thread model** following JUCE convention:

- **Audio Thread (real-time):** Runs the `processBlock()` callback at ~170Hz (44.1kHz / 256 samples). Must complete in under 5.8ms. Absolutely no memory allocation, no mutex locks, no system calls. Reads parameters from atomic values or lock-free FIFOs. Writes audio output and meter data.

- **Message Thread:** Handles everything non-real-time: parameter updates from the Node.js layer, MIDI sequence loading, transport state changes, preset loading. Communicates with the Audio Thread via `juce::AbstractFifo` or single-producer/single-consumer lock-free queues.

**Key classes to expose via N-API:**

| C++ Class | Exposed Functionality | N-API Pattern |
|-----------|-----------------------|---------------|
| `AudioEngine` | `start()`, `stop()`, `getState()` | Persistent instance via `Napi::ObjectWrap` |
| `Transport` | `play()`, `pause()`, `seek(position)`, `setBPM(bpm)` | Method calls on engine instance |
| `TrackManager` | `addTrack()`, `removeTrack()`, `setTrackParam()` | Returns track IDs for reference |
| `MIDISequencer` | `loadClip(trackId, noteData)`, `clearClip()` | Accepts JSON note arrays, converts to internal |
| `MixerBus` | `setVolume()`, `setPan()`, `addEffect()`, `removeEffect()` | Per-track parameter setters |
| `SynthEngine` | `loadPreset()`, `setParameter()` | Per-instrument parameter control |
| `Renderer` | `exportAudio(format, path)` | Async operation via `Napi::AsyncWorker` |
| `MeterBridge` | `getLevels()`, `getPlaybackPosition()` | Polling from Node.js on timer, or callback |

**Lock-free communication pattern:**

```cpp
// Audio Thread reads commands from FIFO
// Message Thread writes commands to FIFO

struct EngineCommand {
    enum Type { SetParam, LoadClip, SetTransport, AddEffect };
    Type type;
    int trackId;
    int paramId;
    float value;
    // ... union or variant for different command data
};

// juce::AbstractFifo for the ring buffer
// Single-producer (message thread) / single-consumer (audio thread)
```

For meter data flowing back (audio thread -> UI), use a separate lock-free FIFO or atomic triple-buffer pattern where the audio thread writes the latest values and the message thread reads the most recent snapshot.

### 2. Native Addon Bridge (N-API / node-addon-api)

**Why node-addon-api (N-API) over alternatives:**
- ABI-stable across Node.js versions (no recompilation per Electron upgrade)
- `Napi::ObjectWrap` cleanly maps C++ engine lifetime to JS object lifetime
- `Napi::AsyncWorker` for non-blocking operations like audio export
- `Napi::ThreadSafeFunction` for callbacks from C++ threads to Node.js event loop

**Architecture of the bridge:**

```
Node.js                          C++ Addon
-------                          ---------
const engine = new AudioEngine() -> constructs JUCE engine
engine.transport.play()          -> calls Transport::play() on message thread
engine.mixer.setVolume(0, -6.0)  -> queues SetParam command to FIFO
engine.getMeterLevels()          -> reads from atomic triple-buffer, returns Float32Array
engine.midi.loadClip(0, notes)   -> converts JS array to C++ note data, queues to FIFO
```

**Critical pattern -- Napi::ThreadSafeFunction for callbacks:**

The C++ message thread needs to push data back to Node.js (meter levels, playback position, transport state changes). Use `Napi::ThreadSafeFunction` to safely invoke JavaScript callbacks from the C++ thread:

```cpp
// C++ side: called from message thread timer
tsfn.NonBlockingCall(meterData, [](Napi::Env env, Napi::Function callback, MeterData* data) {
    auto levels = Napi::Float32Array::New(env, data->numTracks);
    // ... copy level data
    callback.Call({levels});
});
```

**Shared memory for waveform display:**

For large data like audio waveforms, avoid serialization overhead. Use `Napi::ArrayBuffer` wrapping external C++ memory:

```cpp
// C++ allocates waveform summary buffer
// Node.js gets a view into it via ArrayBuffer
// Renderer receives it via IPC (structured clone transfers the buffer)
```

Note: `SharedArrayBuffer` between Electron processes requires cross-origin isolation headers and has complications. Prefer transferring `ArrayBuffer` copies or using the main process as intermediary.

### 3. Project State Model

The Node.js main process owns the **authoritative project state**. This is the single source of truth that the AI reads and the UI displays. The C++ engine receives commands derived from state changes but does not own the project data.

**Why Node.js owns state (not C++):**
- AI needs to read full project state as JSON to include in prompts
- Undo/redo is easier to implement in TypeScript with immutable patterns
- Project save/load is file I/O, natural for Node.js
- C++ engine only needs the "hot" subset (current audio parameters, loaded clips)

**State schema (TypeScript):**

```typescript
interface Project {
  id: string;
  name: string;
  bpm: number;
  timeSignature: [number, number]; // e.g., [4, 4]
  sampleRate: number;
  tracks: Track[];
  masterBus: MixerChannel;
}

interface Track {
  id: string;
  name: string;
  type: 'instrument' | 'audio';
  muted: boolean;
  soloed: boolean;
  mixer: MixerChannel;
  instrument?: InstrumentState;     // synth preset + params
  clips: Clip[];
  effectChain: Effect[];
}

interface MixerChannel {
  volume: number;      // dB
  pan: number;         // -1.0 to 1.0
  sends: Send[];
}

interface Clip {
  id: string;
  startBeat: number;    // position on timeline
  lengthBeats: number;
  type: 'midi' | 'audio';
  midi?: MIDIClipData;
  audio?: AudioClipData;
}

interface MIDIClipData {
  notes: MIDINote[];
}

interface MIDINote {
  pitch: number;        // 0-127
  startBeat: number;    // relative to clip start
  lengthBeats: number;
  velocity: number;     // 0-127
}

interface AudioClipData {
  filePath: string;
  startSample: number;  // offset into source file
  lengthSamples: number;
}

interface Effect {
  id: string;
  type: string;          // 'eq' | 'compressor' | 'reverb' | 'delay' | etc.
  enabled: boolean;
  parameters: Record<string, number>;
}

interface InstrumentState {
  type: string;          // 'subtractive_synth' | 'sampler' | etc.
  preset: string;
  parameters: Record<string, number>;
}

interface Send {
  targetBusId: string;
  amount: number;        // dB
}

interface TransportState {
  playing: boolean;
  recording: boolean;
  positionBeats: number;
  loopEnabled: boolean;
  loopStart: number;
  loopEnd: number;
}
```

**State change flow:**

```
1. Command arrives (from user or AI)
2. Command Dispatcher validates and executes against Project State
3. State updated immutably (new state object)
4. Diff computed and pushed to:
   a. C++ engine (only audio-relevant changes via N-API)
   b. React renderer (full state delta via Electron IPC)
5. Previous state pushed to undo stack
```

### 4. Command Dispatcher (the unifying layer)

Every DAW operation -- whether from UI click, keyboard shortcut, or AI agent -- flows through the same Command Dispatcher. This is the critical architectural chokepoint.

**Command pattern with undo/redo:**

```typescript
interface DAWCommand {
  type: string;
  payload: Record<string, any>;
  execute(state: Project): Project;   // returns new state
  undo(state: Project): Project;      // returns previous state
  describe(): string;                 // human-readable for AI context
}

// Example commands:
// { type: 'ADD_TRACK', payload: { name: 'Bass', type: 'instrument' } }
// { type: 'SET_TRACK_VOLUME', payload: { trackId: 't1', volume: -6.0 } }
// { type: 'WRITE_MIDI_NOTES', payload: { trackId: 't1', clipId: 'c1', notes: [...] } }
// { type: 'ADD_EFFECT', payload: { trackId: 't1', effectType: 'reverb', params: {...} } }
// { type: 'SET_BPM', payload: { bpm: 128 } }
```

**Why a single dispatcher matters:**
- AI and human actions use identical code paths (no special AI bypass)
- Every operation is undoable
- Full operation log for AI context ("here is what has happened so far")
- Validation in one place (reject invalid operations before they reach engine)

### 5. AI Integration Layer

**Architecture:**

```
User message
    |
    v
AI Integration Layer
    |
    +-- Construct prompt:
    |     - System prompt (DAW capabilities, available commands)
    |     - Current project state (serialized from Project State)
    |     - Conversation history
    |     - User message
    |
    +-- Call external API (Claude/GPT/etc. via BYOK key)
    |
    +-- Parse response:
    |     - Extract structured commands (tool calls or JSON blocks)
    |     - Extract conversational text for chat display
    |
    +-- Execute commands via Command Dispatcher (same path as UI)
    |
    +-- Return results to chat UI
```

**AI reads DAW state** by receiving a JSON serialization of the Project state (or a relevant subset) in each prompt. The state schema above is designed to be AI-readable.

**AI writes DAW state** by returning structured commands (using tool-use / function-calling APIs) that map directly to Command Dispatcher operations:

```typescript
// AI tool definitions (provided to Claude/GPT):
const AI_TOOLS = [
  {
    name: 'add_track',
    description: 'Add a new track to the project',
    parameters: {
      name: { type: 'string' },
      type: { type: 'string', enum: ['instrument', 'audio'] },
      instrument: { type: 'string', description: 'Instrument preset name' }
    }
  },
  {
    name: 'write_midi',
    description: 'Write MIDI notes to a clip on a track',
    parameters: {
      trackId: { type: 'string' },
      clipStartBeat: { type: 'number' },
      clipLengthBeats: { type: 'number' },
      notes: {
        type: 'array',
        items: {
          type: 'object',
          properties: {
            pitch: { type: 'number', description: 'MIDI note 0-127' },
            startBeat: { type: 'number' },
            lengthBeats: { type: 'number' },
            velocity: { type: 'number' }
          }
        }
      }
    }
  },
  {
    name: 'set_mixer',
    description: 'Set mixer parameters for a track',
    parameters: {
      trackId: { type: 'string' },
      volume: { type: 'number', description: 'Volume in dB' },
      pan: { type: 'number', description: '-1.0 (left) to 1.0 (right)' }
    }
  },
  {
    name: 'add_effect',
    description: 'Add an effect to a track',
    parameters: {
      trackId: { type: 'string' },
      effectType: { type: 'string', enum: ['eq', 'compressor', 'reverb', 'delay', 'limiter'] },
      parameters: { type: 'object' }
    }
  },
  // ... complete set covers all DAW operations
];
```

**Critical design principle:** The AI tool definitions are a 1:1 mapping to Command Dispatcher commands. There is no separate "AI command" path. The AI is simply another source of the same commands a human user produces.

### 6. React UI Components

**Component architecture:**

| Component | Data Source | Update Frequency | Sync Pattern |
|-----------|------------|-------------------|--------------|
| **Piano Roll** | Project state (MIDI clips) | On edit, on playback cursor move | State subscription + playback position polling |
| **Timeline/Arrangement** | Project state (all clips, tracks) | On edit | State subscription |
| **Mixer** | Project state (volumes, pans) + live meters | Meters: 30-60fps; params: on change | Meters via polling/callback; params via state |
| **Waveform Display** | Computed waveform summary from C++ | On clip load | One-time computation, cached ArrayBuffer transfer |
| **Transport Bar** | Transport state + playback position | Position: 30fps; state: on change | Polling for position, events for state |
| **Chat Interface** | AI conversation state | On message | State subscription |

**Meter/position update loop (high frequency):**

The C++ engine computes meter levels and playback position continuously. The main process polls these at 30-60fps via N-API (`engine.getMeterLevels()`, `engine.getPlaybackPosition()`), then forwards to the renderer via Electron IPC. The renderer uses `requestAnimationFrame` to batch visual updates.

```typescript
// Main process: polling loop
setInterval(() => {
  const levels = engine.getMeterLevels();   // reads atomic triple-buffer
  const position = engine.getPlaybackPosition();
  mainWindow.webContents.send('meter-update', { levels, position });
}, 1000 / 30); // 30fps for meters
```

**State synchronization:**

Use a store pattern (Zustand or similar in the renderer) that mirrors the main process project state. The main process pushes state patches after each command execution:

```
Main Process:  command executed -> new state -> compute diff -> IPC send patch
Renderer:      receive patch -> apply to local store -> React re-renders
```

---

## Patterns to Follow

### Pattern 1: Single-Dispatch Command Architecture
**What:** Every operation (UI, AI, undo, redo, macro) goes through one Command Dispatcher.
**When:** Always. No exceptions.
**Why:** Guarantees consistency, undoability, AI parity with human operations, and auditability.

### Pattern 2: Lock-Free Audio Thread
**What:** The audio callback thread never allocates memory, acquires locks, or makes system calls. All communication via lock-free FIFO queues and atomic values.
**When:** Any data crossing the audio thread boundary.
**Why:** A single stall causes audible glitches. At 5.8ms per buffer, there is zero margin.

### Pattern 3: State Ownership in Node.js, Execution in C++
**What:** The Node.js process owns the project model (tracks, clips, notes, effects). The C++ engine is a stateless executor that receives commands and produces audio.
**When:** For all project data management.
**Why:** Enables trivial JSON serialization for AI prompts, project save/load, undo history. C++ only holds the "hot" audio state it needs for real-time processing.

### Pattern 4: Immutable State Updates
**What:** State changes produce new state objects; old state is retained for undo.
**When:** All project state mutations.
**Why:** Enables undo/redo stack, state diffing for efficient UI updates, and safe concurrent reads.

### Pattern 5: AI as Tool User (Not Privileged Actor)
**What:** AI issues the exact same commands as a human user. No special AI-only APIs.
**When:** All AI interactions with the DAW.
**Why:** Reduces surface area, ensures all AI actions are undoable, prevents AI from putting the system in states a user could not.

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: Locking in the Audio Thread
**What:** Using mutexes, condition variables, or even try_lock in `processBlock()`.
**Why bad:** Priority inversion, unbounded wait times, audible glitches, OS scheduler interference.
**Instead:** Lock-free FIFOs (`juce::AbstractFifo`), atomics, triple-buffering.

### Anti-Pattern 2: C++ Engine Owns Project State
**What:** Making the C++ layer the source of truth for project data (tracks, clips, notes).
**Why bad:** C++ state is hard to serialize to JSON for AI prompts, hard to diff for UI updates, hard to persist for save/load. Creates complex bidirectional sync.
**Instead:** Node.js owns project state; C++ receives commands and reports audio-specific feedback.

### Anti-Pattern 3: Direct Renderer-to-C++ Communication
**What:** Having the React renderer call native addons directly, bypassing the main process.
**Why bad:** Native addons run in the main process in Electron. Renderer process cannot load native addons directly (context isolation). Even if technically possible via worker threads, it breaks the command dispatcher pattern.
**Instead:** All communication flows through the main process command dispatcher.

### Anti-Pattern 4: Synchronous IPC for Meter Data
**What:** Using synchronous `ipcRenderer.sendSync()` for high-frequency meter/position updates.
**Why bad:** Blocks the renderer thread, causes UI jank, defeats the purpose of process separation.
**Instead:** Async IPC with batched updates at 30fps, or MessagePort for lower overhead.

### Anti-Pattern 5: Separate AI Command Path
**What:** Building a parallel set of "AI commands" that bypass the normal command dispatcher.
**Why bad:** AI actions become non-undoable, validation logic is duplicated, AI can put system in invalid states.
**Instead:** AI tool definitions map 1:1 to dispatcher commands.

---

## Scalability Considerations

| Concern | At 8 tracks | At 64 tracks | At 256 tracks |
|---------|------------|--------------|---------------|
| **Audio processing** | Single-threaded fine | Need parallel track processing in audio thread | Must use JUCE's `AudioProcessorGraph` or custom work-stealing |
| **State serialization** | Full state in AI prompts | Summarize state, send only relevant tracks | Hierarchical summaries, AI sees overview + detail on request |
| **IPC bandwidth** | No concern | Meter data grows linearly; batch updates | Consider SharedArrayBuffer or binary protocol for meters |
| **Undo history** | Store full snapshots | Structural sharing (persistent data structures) | Command-based undo (store commands, not full state) |
| **UI rendering** | React handles fine | Virtualize track list, piano roll rows | Canvas-based rendering for piano roll and timeline |

---

## Build Order (Dependency Chain)

The architecture has clear dependency layers. Build from the bottom up:

```
Phase 1: C++ Audio Engine Core
  - Audio I/O (JUCE AudioDeviceManager)
  - Basic DSP graph (single track, simple synth)
  - Transport (play/pause/seek)
  - Lock-free FIFO infrastructure
  No dependency on Node.js or Electron.

Phase 2: Native Addon Bridge
  - node-addon-api wrapper around engine
  - ObjectWrap for engine lifetime
  - Basic method exposure (transport, synth trigger)
  - ThreadSafeFunction for callbacks
  Depends on: Phase 1 (C++ engine exists to wrap)

Phase 3: Electron Shell + Project State
  - Electron app with main process
  - Project state model (TypeScript interfaces)
  - Command Dispatcher (with undo/redo)
  - Basic IPC to renderer
  Depends on: Phase 2 (can call engine from main process)

Phase 4: React UI (Core Views)
  - Timeline/arrangement view
  - Piano roll editor
  - Mixer view
  - Transport controls
  - State subscription and meter polling
  Depends on: Phase 3 (state model and IPC exist)

Phase 5: AI Integration Layer
  - BYOK API key management
  - Prompt construction with project state
  - AI tool definitions matching command dispatcher
  - Chat interface component
  - Response parsing and command execution
  Depends on: Phase 3 (command dispatcher exists for AI to use)
  Can parallel with: Phase 4 (AI and UI are independent consumers of dispatcher)

Phase 6: Built-in Instruments and Effects
  - Subtractive synth, sampler, drum machine
  - EQ, compressor, reverb, delay, limiter
  - Preset system
  Can start in Phase 1 but refinement depends on full pipeline being testable.
```

**Critical path:** Phase 1 -> Phase 2 -> Phase 3 -> (Phase 4 + Phase 5 in parallel) -> Phase 6 threads through all.

---

## Technology-Specific Notes

### JUCE Version
Use JUCE 8.x. It is dual-licensed (AGPLv3 or commercial). For an open-source project, AGPLv3 is fine. JUCE 8 includes improved WebView support and MIDI 2.0, though neither is critical for v1. The core audio engine APIs are mature and stable.

### node-addon-api Version
Use node-addon-api v8+ with N-API version 9+. This ensures compatibility with Electron 28+ and provides stable `ThreadSafeFunction` and `AsyncWorker` APIs.

### Electron Version
Use Electron 33+ (current stable as of early 2026). Key features: context isolation enabled by default, improved IPC performance, Node.js 22+ integration.

### react-juce Consideration
react-juce (Blueprint) embeds a JS engine inside JUCE for rendering React to JUCE components. **Do not use this.** It is unmaintained (last meaningful activity ~2021), uses Duktape (limited ES support), and conflicts with the Electron architecture. LuneyTunes uses Electron's Chromium for React rendering, which is far more capable.

### DAWproject Format
Consider supporting the open [DAWproject](https://github.com/bitwig/dawproject) exchange format (by Bitwig/PreSonus) for import/export interoperability. The project state model above maps naturally to DAWproject's track/clip/note/automation structure.

---

## Sources

- [ElectronMeetsJUCE - JUCE as native addon for Electron](https://github.com/Serge45/ElectronMeetsJUCE) - Proof of concept for JUCE + Electron integration
- [JUCE Forum: Electron/JS app talking to JUCE audio engine](https://forum.juce.com/t/electron-js-app-talking-to-juce-audio-engine/40559)
- [ADC19: Writing apps with JUCE audio backend and JS frontend](https://adc19.sched.com/event/T0VQ/writing-applications-with-juce-audio-back-end-and-javascript-front-end-react-nativeelectron-level-advanced)
- [How to Build a Modern DAW - Misko Lee](https://medium.com/@Misko_Lee/how-to-build-a-modern-digital-audio-workstation-daw-d1f4a2c670e1)
- [Building a Simple AI DAW, Part 2: MCP and Agents](https://jonaylor.com/blog/building-a-simple-ai-daw-part-2-mcp-and-agents)
- [Using locks in real-time audio processing, safely](https://timur.audio/using-locks-in-real-time-audio-processing-safely)
- [JUCE Forum: Lock-free queues and visualization of data](https://forum.juce.com/t/lock-free-queues-and-visualization-of-data/20659)
- [Node-API Documentation](https://nodejs.org/api/n-api.html)
- [node-addon-api GitHub](https://github.com/nodejs/node-addon-api)
- [MessagePorts in Electron](https://www.electronjs.org/docs/latest/tutorial/message-ports)
- [Electron IPC Tutorial](https://www.electronjs.org/docs/latest/tutorial/ipc)
- [DAWproject open exchange format](https://github.com/bitwig/dawproject)
- [KVR: DAW/Sequencer design and architecture discussion](https://www.kvraudio.com/forum/viewtopic.php?t=478572)
- [Building High-Performance Multi-Threaded Audio Processing](https://acestudio.ai/blog/multi-threaded-audio-processing/)
