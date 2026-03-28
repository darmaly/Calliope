# Phase 2: Audio Engine Core - Research

**Researched:** 2026-03-28
**Domain:** JUCE C++ real-time audio engine, multi-track mixing, transport control, lock-free IPC
**Confidence:** HIGH

## Summary

Phase 2 builds the real-time audio processing core on top of Phase 1's JUCE/cmake-js bridge. The existing codebase has a `calliope::Engine` static class with a simple `TestToneCallback` that proves the AudioIODeviceCallback pattern works. This phase must evolve that into a proper multi-track audio engine with: (1) an AudioProcessorGraph-based mixing architecture for multi-track summing and a master bus insert chain, (2) a transport state machine with play/stop/pause/loop/BPM/time-signature control, (3) lock-free FIFO communication between the audio thread and the UI/command thread using JUCE's AbstractFifo, (4) configurable buffer sizes (128-2048), and (5) a metronome click generator synced to transport BPM.

JUCE provides all the building blocks natively: AudioProcessorGraph for the mixing graph, AudioProcessorPlayer to bridge the graph to AudioDeviceManager, AudioPlayHead::PositionInfo for transport state, and AbstractFifo for lock-free communication. The key architectural decision is to use AudioProcessorGraph as the central mixing bus -- each track becomes a node, the master bus is a chain of processor nodes before the output, and the graph handles sample-accurate summing. This avoids hand-rolling a mixer.

**Primary recommendation:** Build the engine around a single AudioProcessorGraph connected to AudioDeviceManager via AudioProcessorPlayer. Implement transport as a custom AudioPlayHead that the graph queries each processBlock. Use AbstractFifo-backed ring buffers for all audio-thread to UI-thread communication. Never allocate, lock, or perform I/O on the audio thread.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None explicitly locked -- this is an infrastructure phase where all implementation choices are at Claude's discretion.

### Claude's Discretion
All implementation choices are at Claude's discretion -- pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1 patterns:
- Async bridge pattern (ThreadSafeFunction + Promise) is established -- extend for new engine APIs
- Atomic primitives for lock-free cross-thread state already proven in test_tone.cpp
- JUCE AudioDeviceManager lifecycle (mutex-guarded setup/teardown) is the existing pattern
- Engine namespace with static methods is the current C++ API surface

### Deferred Ideas (OUT OF SCOPE)
None -- infrastructure phase.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ENG-01 | Audio engine processes multi-track audio in real-time at 44.1kHz/48kHz, 32-bit float internal | AudioProcessorGraph processes in 32-bit float natively; AudioDeviceManager initialise() accepts sample rate; prepareToPlay receives sampleRate parameter |
| ENG-02 | Audio engine supports configurable buffer sizes (128-2048 samples) with stable playback | AudioDeviceManager::setAudioDeviceSetup() accepts bufferSize; AudioDeviceSetup struct has bufferSize field; processBlock handles variable block sizes |
| ENG-04 | Audio routing supports master bus with insert chain for final output processing | AudioProcessorGraph nodes chained in series before output IO processor; graph handles topological ordering automatically |
| ENG-05 | Transport controls: play, stop, pause, loop region, with BPM and time signature control | Custom AudioPlayHead implementation with PositionInfo; atomic transport state read by audio thread; BPM/timeSig/loop stored in PositionInfo |
| ENG-06 | Metronome with toggleable click track and adjustable volume | AudioProcessor node in graph that generates click samples on beat boundaries using PositionInfo::getPpqPosition(); atomic enable/volume controls |
| ARCH-04 | Lock-free FIFO communication between audio thread and UI/command thread | JUCE AbstractFifo with ScopedRead/ScopedWrite; single-producer single-consumer pattern; std::atomic for simple state values |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 (vendored) | Audio engine framework | Already in vendor/JUCE. Provides AudioProcessorGraph, AudioPlayHead, AbstractFifo, DSP primitives |
| juce_audio_processors | (part of JUCE) | AudioProcessor, AudioProcessorGraph, AudioPlayHead | Core abstractions for graph-based audio processing and transport |
| juce_audio_devices | (part of JUCE) | AudioDeviceManager, AudioIODevice | Audio hardware I/O, buffer size configuration |
| juce_audio_basics | (part of JUCE) | AudioBuffer, MidiBuffer, AudioPlayHead::PositionInfo | Buffer types and transport position info |
| juce_dsp | (part of JUCE) | DSP primitives, ProcessorChain | For master bus processing chain and metronome synthesis |
| juce_core | (part of JUCE) | AbstractFifo, threading, atomics | Lock-free FIFO, utility classes |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| node-addon-api | 8.5.0 (installed) | C++ N-API wrapper | Bridge new engine APIs to Node.js |
| cmake-js | 8.0.0 (installed) | CMake native addon build | Already configured in project |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| AudioProcessorGraph | Manual sample-by-sample mixing loop | Graph handles topological sort, channel routing, and buffer management automatically. Manual mixing is error-prone and loses insert chain flexibility. |
| AudioProcessorPlayer | Direct AudioIODeviceCallback (current pattern) | Player handles prepareToPlay/releaseResources lifecycle and MIDI routing. Current TestToneCallback pattern doesn't scale to graph-based processing. |
| AbstractFifo | boost::spsc_queue or custom ring buffer | AbstractFifo is already part of JUCE, no additional dependency. Well-tested for audio use cases. |
| Custom AudioPlayHead | Using AudioProcessor::getPlayHead() from host | We ARE the host. Must implement AudioPlayHead ourselves to provide transport state to processors in the graph. |

**No new installations required.** All dependencies are already in the project from Phase 1. The CMakeLists.txt already links juce_dsp and juce_audio_processors (via juce_audio_formats dependency chain). Need to add `juce_audio_processors` and `juce_audio_utils` explicitly to engine/CMakeLists.txt.

## Architecture Patterns

### Recommended Project Structure
```
engine/
├── include/calliope/
│   ├── engine.h              # Top-level Engine API (exists, extend)
│   ├── audio_graph.h         # AudioProcessorGraph wrapper
│   ├── transport.h           # Transport state machine + AudioPlayHead
│   ├── master_bus.h          # Master bus processor with insert chain
│   ├── metronome.h           # Metronome click AudioProcessor
│   ├── track.h               # Track processor (placeholder for Phase 3+)
│   └── lock_free_queue.h     # AbstractFifo-based typed message queue
├── src/
│   ├── engine.cpp            # Exists -- extend with graph lifecycle
│   ├── test_tone.cpp         # Exists -- retire or keep as fallback
│   ├── audio_graph.cpp       # Graph setup, node management
│   ├── transport.cpp         # Transport state machine
│   ├── master_bus.cpp        # Master bus processor
│   ├── metronome.cpp         # Metronome processor
│   └── lock_free_queue.cpp   # Message queue implementation
native/src/
│   ├── bridge.cpp            # Extend with transport/config bridge functions
│   ├── bridge.h              # Extend declarations
│   └── addon.cpp             # Register new exports
app/src/
│   ├── main/index.ts         # Add IPC handlers for transport, config
│   ├── main/native-bridge.ts # Extend NativeAddon interface
│   ├── preload/index.ts      # Expose new APIs
│   └── renderer/types/calliope.d.ts  # Extend type declarations
```

### Pattern 1: AudioProcessorGraph as Central Mixer
**What:** A single AudioProcessorGraph instance is the engine's audio processing core. All tracks are nodes. The master bus is a chain of nodes before the graph's output IO processor.
**When to use:** Always -- this is the core architecture.
**Example:**
```cpp
// Source: JUCE AudioProcessorGraph docs
// https://docs.juce.com/master/classAudioProcessorGraph.html

class AudioGraph {
public:
    void initialise(double sampleRate, int blockSize) {
        graph.setPlayConfigDetails(0, 2, sampleRate, blockSize);
        graph.prepareToPlay(sampleRate, blockSize);

        // Create IO processors for the graph
        auto audioInput = std::make_unique<
            juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
        auto audioOutput = std::make_unique<
            juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);

        inputNode = graph.addNode(std::move(audioInput));
        outputNode = graph.addNode(std::move(audioOutput));

        // Master bus node sits between tracks and output
        auto masterProc = std::make_unique<MasterBusProcessor>();
        masterNode = graph.addNode(std::move(masterProc));

        // Connect master -> output (stereo)
        graph.addConnection({{masterNode->nodeID, 0}, {outputNode->nodeID, 0}});
        graph.addConnection({{masterNode->nodeID, 1}, {outputNode->nodeID, 1}});
    }

    juce::AudioProcessorGraph::NodeID addTrack(
        std::unique_ptr<juce::AudioProcessor> trackProcessor) {
        auto node = graph.addNode(std::move(trackProcessor));
        // Connect track -> master (stereo)
        graph.addConnection({{node->nodeID, 0}, {masterNode->nodeID, 0}});
        graph.addConnection({{node->nodeID, 1}, {masterNode->nodeID, 1}});
        return node->nodeID;
    }

private:
    juce::AudioProcessorGraph graph;
    juce::AudioProcessorGraph::Node::Ptr inputNode, outputNode, masterNode;
};
```

### Pattern 2: Custom AudioPlayHead for Transport
**What:** The engine implements AudioPlayHead to provide transport state (BPM, time signature, play position, loop points) to all processors in the graph. Transport state is modified from the UI thread via atomics; the audio thread reads it lock-free.
**When to use:** Required -- we are the host, not a plugin.
**Example:**
```cpp
// Source: JUCE AudioPlayHead::PositionInfo docs
// https://docs.juce.com/master/classAudioPlayHead_1_1PositionInfo.html

class Transport : public juce::AudioPlayHead {
public:
    std::optional<PositionInfo> getPosition() const override {
        PositionInfo info;
        info.setTimeInSamples(samplePosition.load());
        info.setBpm(bpm.load());
        info.setTimeSignature({timeSigNum.load(), timeSigDen.load()});
        info.setIsPlaying(playing.load());
        info.setIsLooping(looping.load());

        if (looping.load()) {
            LoopPoints lp;
            lp.ppqStart = loopStartPpq.load();
            lp.ppqEnd = loopEndPpq.load();
            info.setLoopPoints(lp);
        }

        // Calculate PPQ from sample position
        double seconds = static_cast<double>(samplePosition.load()) / sampleRate;
        double beatsPerSecond = bpm.load() / 60.0;
        info.setPpqPosition(seconds * beatsPerSecond);

        return info;
    }

    // Called from audio thread to advance position
    void advancePosition(int numSamples) {
        if (!playing.load()) return;

        int64_t newPos = samplePosition.load() + numSamples;

        // Handle loop
        if (looping.load()) {
            double loopEndSamples = (loopEndPpq.load() / (bpm.load() / 60.0)) * sampleRate;
            if (newPos >= static_cast<int64_t>(loopEndSamples)) {
                double loopStartSamples = (loopStartPpq.load() / (bpm.load() / 60.0)) * sampleRate;
                newPos = static_cast<int64_t>(loopStartSamples);
            }
        }

        samplePosition.store(newPos);
    }

    // UI thread setters
    void play()  { playing.store(true); }
    void stop()  { playing.store(false); samplePosition.store(0); }
    void pause() { playing.store(false); }  // Don't reset position
    void setBpm(double newBpm) { bpm.store(newBpm); }

private:
    std::atomic<int64_t> samplePosition{0};
    std::atomic<double> bpm{120.0};
    std::atomic<int> timeSigNum{4};
    std::atomic<int> timeSigDen{4};
    std::atomic<bool> playing{false};
    std::atomic<bool> looping{false};
    std::atomic<double> loopStartPpq{0.0};
    std::atomic<double> loopEndPpq{0.0};
    double sampleRate = 44100.0;
};
```

### Pattern 3: Lock-Free Message Queue (AbstractFifo)
**What:** Typed message queue built on AbstractFifo for sending structured commands from the UI thread to the audio thread (and meter data back).
**When to use:** Any structured data exchange between audio and non-audio threads.
**Example:**
```cpp
// Source: JUCE AbstractFifo docs
// https://docs.juce.com/master/classAbstractFifo.html

template <typename T, int Size>
class LockFreeQueue {
public:
    LockFreeQueue() : fifo(Size) {}

    bool push(const T& item) {
        const auto scope = fifo.write(1);
        if (scope.blockSize1 > 0) {
            buffer[scope.startIndex1] = item;
            return true;
        }
        return false;  // Queue full
    }

    bool pop(T& item) {
        const auto scope = fifo.read(1);
        if (scope.blockSize1 > 0) {
            item = buffer[scope.startIndex1];
            return true;
        }
        return false;  // Queue empty
    }

private:
    juce::AbstractFifo fifo;
    std::array<T, Size> buffer;
};
```

### Pattern 4: Metronome as AudioProcessor Node
**What:** A lightweight AudioProcessor that generates click sounds on beat boundaries, reading transport position from AudioPlayHead.
**When to use:** ENG-06 requirement.
**Example:**
```cpp
class MetronomeProcessor : public juce::AudioProcessor {
public:
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&) override {
        if (!enabled.load()) {
            buffer.clear();
            return;
        }

        auto* playHead = getPlayHead();
        if (!playHead) { buffer.clear(); return; }

        auto posInfo = playHead->getPosition();
        if (!posInfo || !posInfo->getIsPlaying()) {
            buffer.clear();
            return;
        }

        double ppq = posInfo->getPpqPosition().orFallback(0.0);
        double bpm = posInfo->getBpm().orFallback(120.0);

        // Generate click on beat boundaries
        double samplesPerBeat = (60.0 / bpm) * getSampleRate();
        // ... synthesize click envelope at beat positions ...

        // Apply volume
        buffer.applyGain(volume.load());
    }

    std::atomic<bool> enabled{true};
    std::atomic<float> volume{0.7f};
};
```

### Pattern 5: Engine Lifecycle (AudioDeviceManager + AudioProcessorPlayer)
**What:** Replace the current static TestToneCallback approach with AudioProcessorPlayer connected to AudioProcessorGraph, managed by AudioDeviceManager.
**When to use:** Core engine initialization/teardown.
**Example:**
```cpp
class AudioGraph {
public:
    bool initialise(double sampleRate, int bufferSize) {
        juce::MessageManager::getInstance();

        // Configure device
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        setup.sampleRate = sampleRate;
        setup.bufferSize = bufferSize;
        setup.outputChannels.setRange(0, 2, true);  // Stereo output

        auto error = deviceManager.initialise(0, 2, nullptr, true, {}, &setup);
        if (error.isNotEmpty()) return false;

        // Set the graph as the playhead for all processors
        graph.setPlayHead(&transport);

        // Connect player to graph, then to device
        player.setProcessor(&graph);
        deviceManager.addAudioCallback(&player);

        return true;
    }

    void shutdown() {
        deviceManager.removeAudioCallback(&player);
        player.setProcessor(nullptr);
        deviceManager.closeAudioDevice();
    }

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer player;
    juce::AudioProcessorGraph graph;
    Transport transport;
};
```

### Anti-Patterns to Avoid
- **Allocating memory on the audio thread:** Never use `new`, `std::vector::push_back`, or any STL container that may allocate inside processBlock. Pre-allocate everything in prepareToPlay.
- **Locking on the audio thread:** Never use std::mutex, std::lock_guard, or any blocking primitive in processBlock. Use std::atomic for single values and AbstractFifo for structured data.
- **System calls on the audio thread:** No file I/O, no logging, no console output from processBlock. These can block for milliseconds and cause audio glitches.
- **Using `std::atomic<double>` for complex state:** On some platforms, atomic<double> may not be lock-free. Check `std::atomic<double>::is_always_lock_free`. If not, use atomic<float> or pack state into a single atomic<uint64_t>.
- **Recreating AudioDeviceManager per operation:** The current test_tone.cpp creates/destroys the device manager for each tone. The engine must have a single long-lived instance.
- **Static class with static state:** The current `calliope::Engine` uses static methods and file-scoped static variables. This should evolve into a proper singleton or instance-based design for the audio graph lifecycle.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Multi-track audio mixing | Manual sample-by-sample loop summing channels | `juce::AudioProcessorGraph` | Handles topological sort, buffer allocation, channel routing, and latency compensation. A manual mixer misses edge cases (feedback loops, channel count mismatches). |
| Lock-free FIFO | Custom ring buffer with atomics | `juce::AbstractFifo` with ScopedRead/ScopedWrite | Battle-tested, handles wraparound correctly, RAII scoped access prevents finishedRead/Write mismatches. |
| Transport position tracking | Manual sample counting with PPQ math | `juce::AudioPlayHead::PositionInfo` | Standard struct that all JUCE processors understand. Implement AudioPlayHead interface, let framework distribute position. |
| Audio device management | Direct CoreAudio/WASAPI/ALSA calls | `juce::AudioDeviceManager` | Cross-platform abstraction. Handles device enumeration, sample rate negotiation, buffer size configuration, and hot-plug events. |
| Connecting graph to hardware | Custom AudioIODeviceCallback wrapping the graph | `juce::AudioProcessorPlayer` | Bridges AudioProcessor (graph) to AudioIODeviceCallback (device). Handles prepareToPlay/releaseResources lifecycle automatically. |
| Click sound synthesis | Loading WAV samples for metronome | `juce::dsp::Oscillator` + envelope | Simple sine/triangle burst is cleaner than managing sample files. No file I/O dependency. |

**Key insight:** JUCE's audio module ecosystem is designed for exactly this use case. Every component we need (graph mixer, transport, lock-free queue, device management) has a well-tested JUCE implementation. The work is wiring them together correctly, not building primitives.

## Common Pitfalls

### Pitfall 1: Priority Inversion on Audio Thread
**What goes wrong:** UI thread holds a mutex, audio thread tries to acquire it, audio thread blocks waiting for UI thread which is lower priority. Audio glitches occur.
**Why it happens:** Developers use std::mutex for "quick" shared state access, forgetting the audio thread has real-time constraints.
**How to avoid:** Use std::atomic for single values. Use AbstractFifo for structured messages. Only hold std::mutex during setup/teardown when the audio callback is not active.
**Warning signs:** Intermittent clicks/pops that correlate with UI activity (resizing windows, opening dialogs).

### Pitfall 2: AudioProcessorGraph Update Blocking
**What goes wrong:** Calling `addNode()` or `addConnection()` with default `UpdateKind::sync` triggers a graph rebuild that blocks the audio thread.
**Why it happens:** The default sync mode rebuilds immediately, which is fine for setup but problematic during playback.
**How to avoid:** Use `UpdateKind::async` for topology changes during playback. Batch multiple changes and call `rebuild()` once with `UpdateKind::none` + manual `rebuild()`.
**Warning signs:** Audio dropout when adding/removing tracks during playback.

### Pitfall 3: Buffer Size Mismatch
**What goes wrong:** Processors assume a fixed buffer size equal to the configured value, but JUCE may deliver smaller blocks (especially the last block before stopping).
**Why it happens:** The buffer size in AudioDeviceSetup is a maximum/preferred size, not guaranteed per-callback.
**How to avoid:** Always use the `numSamples` parameter from processBlock, never cache and assume. The prepareToPlay `maximumExpectedSamplesPerBlock` is an upper bound only.
**Warning signs:** Crashes or garbage audio at playback start/stop boundaries.

### Pitfall 4: Sample Rate Not Propagated
**What goes wrong:** Transport calculates PPQ positions using a stale sample rate after the user changes audio device settings.
**Why it happens:** Sample rate is set in prepareToPlay but transport stores its own copy that isn't updated.
**How to avoid:** The transport's sampleRate must be updated in prepareToPlay. Since Transport is the AudioPlayHead, and the graph calls prepareToPlay on all processors, add a setSampleRate method called from the graph's prepareToPlay.
**Warning signs:** Metronome drifts from actual beat positions; loop points are wrong after device change.

### Pitfall 5: MessageManager Lifetime in Electron
**What goes wrong:** JUCE's AudioDeviceManager requires a MessageManager instance. In Electron's Node.js addon context, there's no JUCE application event loop.
**Why it happens:** JUCE expects its own message loop for async callbacks and device change notifications.
**How to avoid:** Call `juce::MessageManager::getInstance()` early in engine initialization (already done in Phase 1). For device change callbacks, use JUCE's `callAsync` to bounce to the message thread, then use ThreadSafeFunction to bounce to Node.js.
**Warning signs:** Crashes on device hot-plug, or device enumeration returning stale data.

### Pitfall 6: Atomic Double Not Lock-Free
**What goes wrong:** `std::atomic<double>` operations silently use a mutex on some ARM platforms, causing audio thread blocking.
**Why it happens:** Not all architectures support 8-byte atomic operations natively.
**How to avoid:** Use `static_assert(std::atomic<double>::is_always_lock_free)` in transport.h. If it fails, use `std::atomic<float>` for BPM/volume or pack into `std::atomic<uint64_t>` with bit_cast.
**Warning signs:** Platform-specific glitches on less common architectures (unlikely on x86_64 macOS/Windows but possible on ARM Linux).

## Code Examples

### Engine API Surface (Bridge Functions)
```cpp
// New bridge functions to add to native/src/bridge.cpp
// Following established ThreadSafeFunction + Promise pattern

Napi::Value InitialiseEngine(const Napi::CallbackInfo& info);  // sampleRate, bufferSize
Napi::Value ShutdownEngine(const Napi::CallbackInfo& info);
Napi::Value TransportPlay(const Napi::CallbackInfo& info);
Napi::Value TransportStop(const Napi::CallbackInfo& info);
Napi::Value TransportPause(const Napi::CallbackInfo& info);
Napi::Value SetBpm(const Napi::CallbackInfo& info);            // bpm: number
Napi::Value SetTimeSignature(const Napi::CallbackInfo& info);  // num: int, den: int
Napi::Value SetLoopRegion(const Napi::CallbackInfo& info);     // startBeat, endBeat, enabled
Napi::Value SetBufferSize(const Napi::CallbackInfo& info);     // bufferSize: int
Napi::Value SetMetronomeEnabled(const Napi::CallbackInfo& info);  // enabled: bool
Napi::Value SetMetronomeVolume(const Napi::CallbackInfo& info);   // volume: float
Napi::Value GetTransportState(const Napi::CallbackInfo& info); // returns position, bpm, playing, etc.
Napi::Value GetAudioConfig(const Napi::CallbackInfo& info);    // returns sampleRate, bufferSize, device
```

### IPC Channel Naming Convention
```typescript
// Follow existing 'engine:*' namespace pattern from Phase 1
// app/src/main/index.ts additions:

ipcMain.handle('engine:initialise', async (_event, sampleRate: number, bufferSize: number) => {
  return await native.initialiseEngine(sampleRate, bufferSize)
})
ipcMain.handle('engine:shutdown', async () => {
  return await native.shutdownEngine()
})
ipcMain.handle('engine:transport:play', async () => {
  return await native.transportPlay()
})
ipcMain.handle('engine:transport:stop', async () => {
  return await native.transportStop()
})
ipcMain.handle('engine:transport:pause', async () => {
  return await native.transportPause()
})
ipcMain.handle('engine:transport:setBpm', async (_event, bpm: number) => {
  return await native.setBpm(bpm)
})
ipcMain.handle('engine:transport:setTimeSignature', async (_event, num: number, den: number) => {
  return await native.setTimeSignature(num, den)
})
ipcMain.handle('engine:transport:setLoop', async (_event, startBeat: number, endBeat: number, enabled: boolean) => {
  return await native.setLoopRegion(startBeat, endBeat, enabled)
})
ipcMain.handle('engine:config:setBufferSize', async (_event, bufferSize: number) => {
  return await native.setBufferSize(bufferSize)
})
ipcMain.handle('engine:metronome:setEnabled', async (_event, enabled: boolean) => {
  return await native.setMetronomeEnabled(enabled)
})
ipcMain.handle('engine:metronome:setVolume', async (_event, volume: number) => {
  return await native.setMetronomeVolume(volume)
})
ipcMain.handle('engine:transport:getState', async () => {
  return await native.getTransportState()
})
ipcMain.handle('engine:config:getAudioConfig', async () => {
  return await native.getAudioConfig()
})
```

### CMakeLists.txt Addition
```cmake
# engine/CMakeLists.txt -- add juce_audio_processors and new source files
add_library(calliope_engine STATIC
    src/engine.cpp
    src/test_tone.cpp
    src/audio_graph.cpp
    src/transport.cpp
    src/master_bus.cpp
    src/metronome.cpp
    src/lock_free_queue.cpp
)

target_include_directories(calliope_engine PUBLIC include)

target_link_libraries(calliope_engine
    PUBLIC
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors   # NEW: for AudioProcessorGraph
        juce::juce_audio_utils        # NEW: for AudioProcessorPlayer
        juce::juce_events
        juce::juce_dsp
)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `AudioPlayHead::getCurrentPosition()` with `CurrentPositionInfo` | `AudioPlayHead::getPosition()` returning `std::optional<PositionInfo>` | JUCE 7.0+ | PositionInfo uses Optional types per field, safer API. Old API is deprecated. |
| `AudioProcessorGraph::addNode` (sync only) | `addNode` with `UpdateKind` parameter (sync/async/none) | JUCE 7.0+ | Allows non-blocking graph topology changes during playback |
| `AbstractFifo` manual prepareToRead/finishedRead | `ScopedRead` / `ScopedWrite` RAII wrappers | JUCE 6.0+ | Prevents forgetting to call finishedRead/finishedWrite |

**Deprecated/outdated:**
- `AudioPlayHead::getCurrentPosition()`: Use `getPosition()` instead. Returns `std::optional<PositionInfo>`.
- `CurrentPositionInfo` struct: Use `PositionInfo` class with getter/setter methods.
- `AbstractFifo::prepareToRead/prepareToWrite` manual pattern: Prefer `ScopedRead`/`ScopedWrite` for RAII safety.

## Open Questions

1. **Thread management for AudioProcessorPlayer in Electron context**
   - What we know: Phase 1 proved MessageManager::getInstance() works. AudioDeviceManager runs its audio callback on a dedicated high-priority thread managed by the OS audio subsystem (CoreAudio thread on macOS).
   - What's unclear: Whether AudioProcessorPlayer's internal threading interacts poorly with Node.js's libuv event loop when both are running in the same process.
   - Recommendation: Test early. If issues arise, AudioProcessorPlayer is a thin wrapper -- can implement AudioIODeviceCallback directly on the graph (call graph.processBlock from callback).

2. **Graph rebuild during buffer size change**
   - What we know: Changing buffer size requires device restart (closeAudioDevice + initialise with new setup). prepareToPlay is called again on the graph.
   - What's unclear: Whether this causes an audible gap even with proper lifecycle management.
   - Recommendation: Accept a brief silence during buffer size change. Document that buffer size changes should happen during pause, not playback. This is standard DAW behavior.

3. **How to evolve static Engine class**
   - What we know: Current `calliope::Engine` is a static class with static methods. The audio graph needs instance state (AudioDeviceManager, AudioProcessorGraph, Transport).
   - What's unclear: Whether to make Engine a proper singleton or use a module-level instance.
   - Recommendation: Convert to a singleton pattern (`Engine::getInstance()`) that owns the AudioGraph. Bridge functions call through the singleton. This preserves the static API feel while allowing instance state.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | C++ build | Yes | 4.3.0 | -- |
| Apple Clang | C++ compilation | Yes | 17.0.0 | -- |
| JUCE | Audio engine | Yes (vendored) | 8.0.12 | -- |
| node-addon-api | Native bridge | Yes (installed) | 8.5.0 | -- |
| cmake-js | Build native addon | Yes (installed) | 8.0.0 | -- |
| pnpm | Package manager | Yes | 10.33.0 | -- |
| Vitest | TypeScript tests | Yes | 3.2.4 | -- |
| Catch2 | C++ unit tests | No | -- | FetchContent in CMake or header-only download |

**Missing dependencies with fallback:**
- Catch2 is not installed system-wide. Use CMake FetchContent to pull Catch2 v3.x as part of the build, or add as a vendored header-only include. This is standard practice for C++ projects.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework (TS) | Vitest 3.2.4 |
| Framework (C++) | Catch2 v3 (needs setup via FetchContent) |
| Config file (TS) | vitest.config.ts (via package.json scripts) |
| Config file (C++) | None -- Wave 0 gap |
| Quick run command (TS) | `pnpm test` |
| Quick run command (C++) | `cmake --build build --target calliope_engine_tests && ./build/engine/tests/calliope_engine_tests` |
| Full suite command | `pnpm test && cmake --build build --target calliope_engine_tests && ./build/engine/tests/calliope_engine_tests` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ENG-01 | Audio graph outputs 32-bit float at 44.1/48kHz | integration (C++) | C++ test: verify AudioBuffer<float> format and sample rate in processBlock | No -- Wave 0 |
| ENG-02 | Buffer size configurable 128-2048 without glitch | integration (C++) | C++ test: initialise with various buffer sizes, verify callback receives correct numSamples | No -- Wave 0 |
| ENG-04 | Master bus processes through insert chain | unit (C++) | C++ test: add processor to master chain, verify signal passes through | No -- Wave 0 |
| ENG-05 | Transport play/stop/pause/loop with BPM/timeSig | unit (C++) | C++ test: verify Transport state transitions, PositionInfo correctness | No -- Wave 0 |
| ENG-06 | Metronome clicks in sync with BPM | unit (C++) | C++ test: verify MetronomeProcessor generates samples at beat boundaries | No -- Wave 0 |
| ARCH-04 | Lock-free FIFO audio-thread to UI-thread | unit (C++) | C++ test: push/pop from LockFreeQueue, verify SPSC correctness | No -- Wave 0 |
| Bridge | New bridge functions callable from Node.js | integration (TS) | `pnpm test:native` -- extend native.test.ts | Partial (todo stubs) |
| IPC | New IPC channels work end-to-end | integration (TS) | `pnpm test:app` -- extend app.test.ts | Partial (todo stubs) |

### Sampling Rate
- **Per task commit:** `pnpm test` (TypeScript) + C++ unit test binary
- **Per wave merge:** Full suite (both TS and C++)
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `engine/tests/CMakeLists.txt` -- Catch2 test target setup via FetchContent
- [ ] `engine/tests/test_transport.cpp` -- covers ENG-05
- [ ] `engine/tests/test_lock_free_queue.cpp` -- covers ARCH-04
- [ ] `engine/tests/test_metronome.cpp` -- covers ENG-06
- [ ] `engine/tests/test_audio_graph.cpp` -- covers ENG-01, ENG-02, ENG-04
- [ ] `test/native.test.ts` -- extend existing todo stubs for new bridge functions
- [ ] `test/app.test.ts` -- extend existing todo stubs for new IPC handlers

## Project Constraints (from CLAUDE.md)

- **Audio Engine**: C++ with JUCE framework -- real-time audio requirements demand native performance
- **Audio Quality**: Professional-grade -- 44.1kHz/48kHz sample rates, 32-bit float internal processing minimum
- **Native Bridge**: node-addon-api + cmake-js (established in Phase 1)
- **State Management**: Zustand (not relevant to this phase but don't introduce alternatives)
- **Testing C++**: Catch2 (per CLAUDE.md Development Tools)
- **Testing TS**: Vitest (per CLAUDE.md Development Tools)
- **Package Manager**: pnpm
- **C++ Standard**: C++17 minimum
- **JUCE Modules**: Use juce_audio_processors, juce_dsp, juce_audio_basics, juce_audio_devices, juce_events, juce_core
- **GSD Workflow**: All edits through GSD commands

## Sources

### Primary (HIGH confidence)
- [JUCE AudioProcessorGraph Class Reference](https://docs.juce.com/master/classAudioProcessorGraph.html) -- Graph-based audio processing, addNode/addConnection API, UpdateKind modes
- [JUCE AudioPlayHead::PositionInfo Class Reference](https://docs.juce.com/master/classAudioPlayHead_1_1PositionInfo.html) -- Modern transport position API with Optional types
- [JUCE AbstractFifo Class Reference](https://docs.juce.com/master/classAbstractFifo.html) -- Lock-free FIFO with ScopedRead/ScopedWrite
- [JUCE AudioProcessorPlayer Class Reference](https://docs.juce.com/master/classAudioProcessorPlayer.html) -- Bridges AudioProcessor to AudioIODeviceCallback
- [JUCE Cascading Plug-in Effects Tutorial](https://juce.com/tutorials/tutorial_audio_processor_graph/) -- AudioProcessorGraph in standalone context

### Secondary (MEDIUM confidence)
- [JUCE Forum: Lock-free queues and visualization](https://forum.juce.com/t/lock-free-queues-and-visualization-of-data/20659) -- Community patterns for FIFO usage
- [JUCE Forum: AbstractFifo thread safety](https://forum.juce.com/t/abstractfifo-single-consumer-single-producer-thread-safety/50749) -- SPSC guarantees confirmed
- [timur.audio: Using locks in real-time audio processing](https://timur.audio/using-locks-in-real-time-audio-processing-safely) -- Comprehensive rules for audio thread safety
- [JUCE Forum: Reading/writing values lock free to/from processBlock](https://forum.juce.com/t/reading-writing-values-lock-free-to-from-processblock/50947) -- std::atomic patterns

### Tertiary (LOW confidence)
- None -- all findings verified against JUCE official documentation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All components are JUCE built-ins already vendored in the project
- Architecture: HIGH -- AudioProcessorGraph + AudioProcessorPlayer + custom AudioPlayHead is the standard JUCE pattern for standalone DAW-like applications
- Pitfalls: HIGH -- Well-documented in JUCE community; real-time audio thread rules are universal

**Research date:** 2026-03-28
**Valid until:** 2026-06-28 (JUCE 8 is stable, patterns unlikely to change)
