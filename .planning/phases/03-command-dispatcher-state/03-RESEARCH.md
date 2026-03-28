# Phase 3: Command Dispatcher & State - Research

**Researched:** 2026-03-28
**Domain:** Command pattern architecture, undo/redo, JSON state serialization, event emission
**Confidence:** HIGH

## Summary

Phase 3 wraps every DAW operation in a command pattern, enabling undo/redo, event emission, and JSON-serializable state. The existing codebase has direct method calls from bridge to engine (e.g., `Engine::getInstance().transportPlay()`). This phase interposes a `CommandDispatcher` that receives command objects, executes them on the engine, records them for undo/redo, emits state-change events, and maintains a serializable project state tree.

The architecture is entirely custom C++ -- no external libraries needed beyond what JUCE already provides. The command pattern, undo stack, event system, and JSON serialization are all straightforward to implement with JUCE's built-in `juce::var`, `juce::DynamicObject`, `juce::JSON`, and `juce::UndoManager` classes. The key design challenge is making commands invertible (for undo) while keeping the audio thread lock-free.

**Primary recommendation:** Implement a `CommandDispatcher` class that lives on the UI/command thread, uses JUCE's `juce::UndoManager` for undo/redo history, serializes project state via `juce::JSON`, and emits events via a listener/callback pattern that the native bridge forwards to Electron.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None explicitly locked -- all implementation choices at Claude's discretion.

### Claude's Discretion
All implementation choices are at Claude's discretion -- pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1-2 patterns:
- Engine is a singleton (Engine::getInstance()) -- dispatcher should integrate with this
- Lock-free communication via LockFreeQueue is established -- use for command dispatch to audio thread
- Transport already has atomic state -- transport commands should wrap existing Transport methods
- Native bridge uses ThreadSafeFunction + Promise pattern -- extend for command dispatch APIs
- IPC uses 'engine:*' namespace -- extend for 'command:*' or similar

### Deferred Ideas (OUT OF SCOPE)
None.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ARCH-01 | Single command dispatcher -- all operations (UI, future AI) flow through one interface | CommandDispatcher class as sole entry point; bridge functions route through it instead of calling Engine directly |
| ARCH-02 | DAW state serializable to JSON -- full project state readable by external consumers | ProjectState class with toJson()/fromJson() using juce::JSON; state tree covers transport, tracks, mixer, config |
| ARCH-05 | Every instrument and effect parameter addressable by ID via command dispatcher | Parameter registry with string IDs (e.g., "transport.bpm", "master.volume"); SetParameterCommand addresses any registered param |
| ARCH-06 | Command dispatcher emits events for all state changes | Listener pattern on CommandDispatcher; bridge forwards events via ThreadSafeFunction to Electron IPC |
| PROJ-03 | Undo/redo with deep history stack (minimum 100 operations) | juce::UndoManager with configurable history depth; each Command implements undo(); stack tracks 100+ operations |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE juce_core | 8.0.12 | JSON serialization (juce::JSON, juce::var, juce::DynamicObject) | Already in the project. juce::JSON::toString/parse handles the JSON requirement without adding nlohmann or rapidjson. |
| JUCE juce_data_structures | 8.0.12 | juce::UndoManager, juce::UndoableAction | JUCE's built-in undo system. UndoManager tracks action history with configurable depth. UndoableAction is the base class for invertible commands. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| node-addon-api | 8.7.0 | Bridge command dispatch APIs to Node.js | Already in project. Extend bridge.cpp with command dispatch and event subscription functions. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| juce::UndoManager | Custom undo stack | UndoManager is battle-tested, handles transaction grouping, configurable history size. No reason to hand-roll. |
| juce::JSON | nlohmann/json | Adding a new dependency when JUCE already provides JSON. juce::var is less ergonomic than nlohmann but avoids dependency. |
| Listener pattern | std::function callbacks | Listener pattern (JUCE ChangeBroadcaster or custom) is more structured and supports multiple subscribers. |

**Note on juce_data_structures:** This module is NOT currently linked in `engine/CMakeLists.txt`. It must be added to `target_link_libraries` for `juce::juce_data_structures` to access `juce::UndoManager` and `juce::UndoableAction`.

## Architecture Patterns

### Recommended Project Structure
```
engine/
  include/calliope/
    command_dispatcher.h    # CommandDispatcher class (singleton-adjacent, owned by Engine)
    command.h               # Base Command class (extends juce::UndoableAction)
    commands/
      transport_commands.h  # PlayCommand, StopCommand, SetBpmCommand, etc.
      parameter_commands.h  # SetParameterCommand (generic, ID-addressed)
    project_state.h         # JSON-serializable project state tree
    parameter_registry.h    # Maps string IDs to parameter pointers
    event_types.h           # Event type enum and event data structs
  src/
    command_dispatcher.cpp
    commands/
      transport_commands.cpp
      parameter_commands.cpp
    project_state.cpp
    parameter_registry.cpp
native/src/
    bridge.cpp              # Extend with command:* functions
app/src/
    main/index.ts           # Extend with command:* IPC handlers
    preload/index.ts        # Expose command APIs to renderer
```

### Pattern 1: Command Pattern via juce::UndoableAction
**What:** Every operation is a subclass of `juce::UndoableAction` with `perform()` and `undo()` methods. The dispatcher calls `UndoManager::perform(action)` which executes the action and adds it to the history.
**When to use:** All state-changing operations.
**Example:**
```cpp
// Command base that adds event emission
class Command : public juce::UndoableAction {
public:
    virtual ~Command() = default;
    virtual juce::String getCommandName() const = 0;
    virtual juce::var getEventData() const = 0;
};

class SetBpmCommand : public Command {
public:
    SetBpmCommand(double newBpm) : newBpm_(newBpm) {}

    bool perform() override {
        auto& transport = Engine::getInstance().getTransport();
        oldBpm_ = transport.getBpm();
        transport.setBpm(newBpm_);
        return true;  // true = action was performed
    }

    bool undo() override {
        Engine::getInstance().getTransport().setBpm(oldBpm_);
        return true;
    }

    int getSizeInUnits() override { return 1; }

    juce::String getCommandName() const override { return "SetBpm"; }

    juce::var getEventData() const override {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("bpm", newBpm_);
        return juce::var(obj);
    }

private:
    double newBpm_;
    double oldBpm_ = 120.0;
};
```

### Pattern 2: CommandDispatcher as Central Hub
**What:** Single class that accepts commands, executes via UndoManager, emits events, and updates project state.
**When to use:** All operations route through this.
**Example:**
```cpp
class CommandDispatcher {
public:
    // Execute a command -- takes ownership
    bool dispatch(std::unique_ptr<Command> command);

    // Undo/redo
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

    // Event subscription
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void commandExecuted(const Command& cmd) = 0;
        virtual void commandUndone(const Command& cmd) = 0;
    };
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // State
    juce::var getProjectState() const;

private:
    juce::UndoManager undoManager_{0, 200}; // 0 = no size limit, 200 actions max
    juce::ListenerList<Listener> listeners_;
};
```

### Pattern 3: Parameter Registry for ID-Based Addressing (ARCH-05)
**What:** A registry that maps string IDs (e.g., "transport.bpm", "metronome.volume", "master.volume") to getter/setter function pairs. The `SetParameterCommand` uses this to address any parameter by ID.
**When to use:** When the AI layer or any external consumer needs to set a parameter by name.
**Example:**
```cpp
class ParameterRegistry {
public:
    struct ParameterDef {
        std::function<juce::var()> getter;
        std::function<void(const juce::var&)> setter;
        juce::String type;  // "float", "double", "int", "bool"
        juce::var minValue;
        juce::var maxValue;
    };

    void registerParameter(const juce::String& id, ParameterDef def);
    const ParameterDef* getParameter(const juce::String& id) const;
    std::vector<juce::String> getAllParameterIds() const;

private:
    std::map<juce::String, ParameterDef> params_;
};
```

### Pattern 4: Event Emission via Native Bridge
**What:** The bridge registers a `CommandDispatcher::Listener` that forwards events to Node.js via `ThreadSafeFunction`. Electron's main process re-emits these to the renderer via IPC.
**When to use:** ARCH-06 requirement -- all state changes emit events.
**Example:**
```cpp
// In bridge.cpp -- register a persistent TSFN for events
static Napi::ThreadSafeFunction eventCallback;

class BridgeListener : public CommandDispatcher::Listener {
public:
    void commandExecuted(const Command& cmd) override {
        auto name = cmd.getCommandName().toStdString();
        auto data = juce::JSON::toString(cmd.getEventData()).toStdString();
        eventCallback.BlockingCall([name, data](Napi::Env env, Napi::Function) {
            // Emit to Node.js
        });
    }
};
```

### Pattern 5: Project State Tree (ARCH-02)
**What:** A `ProjectState` class that mirrors all engine state as a JSON-serializable tree. Updated after every command execution.
**When to use:** State serialization, save/load, AI state reading.
**Example:**
```cpp
class ProjectState {
public:
    juce::var toJson() const;
    void fromJson(const juce::var& json);

    // Sub-state accessors
    struct TransportState {
        juce::String state;  // "stopped", "playing", "paused"
        double bpm = 120.0;
        int timeSigNum = 4;
        int timeSigDen = 4;
        bool looping = false;
        double loopStart = 0.0;
        double loopEnd = 0.0;
    };

    struct MetronomeState {
        bool enabled = true;
        float volume = 0.7f;
    };

    struct MasterBusState {
        float volume = 1.0f;
    };

    struct AudioConfigState {
        double sampleRate = 44100.0;
        int bufferSize = 512;
    };

    TransportState transport;
    MetronomeState metronome;
    MasterBusState masterBus;
    AudioConfigState audioConfig;
    // Future phases add: tracks[], instruments[], effects[]
};
```

### Anti-Patterns to Avoid
- **Direct engine method calls from bridge:** Phase 2 calls `Engine::getInstance().transportPlay()` directly. Phase 3 must route ALL operations through the dispatcher. The bridge functions should create command objects and dispatch them.
- **Undo for non-undoable operations:** Transport play/stop/pause are not meaningfully undoable (you don't "undo" pressing play). Mark these as non-undoable commands that still emit events but skip the undo stack.
- **Blocking the audio thread:** Commands execute on the UI/command thread. If a command needs to affect the audio thread (e.g., setting BPM), it uses the existing atomic pattern (Transport atomics) or the LockFreeQueue. Never lock a mutex from the audio thread.
- **Monolithic command class:** Don't put all command logic in one giant switch statement. Each command is its own class, keeping the codebase extensible for Phases 4-9.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Undo/redo stack | Custom linked list of operations | juce::UndoManager + juce::UndoableAction | Handles transaction grouping, configurable depth, memory management. Battle-tested in DAW apps. |
| JSON serialization | Manual string concatenation | juce::JSON + juce::var + juce::DynamicObject | juce::var is a variant type that maps naturally to JSON. JSON::toString/fromString handle parsing. |
| Event broadcasting | Raw function pointer arrays | juce::ListenerList<T> | Thread-safe listener management, automatic dead-listener cleanup. |

**Key insight:** JUCE provides nearly everything needed for this phase. The `juce_data_structures` module has `UndoManager` and `UndoableAction` designed exactly for this use case. The `juce_core` module has `JSON`, `var`, `DynamicObject`, and `ListenerList`. Adding external dependencies would be wasteful.

## Common Pitfalls

### Pitfall 1: Audio Thread Safety with Commands
**What goes wrong:** A command tries to allocate memory, lock a mutex, or do I/O on the audio thread, causing glitches or deadlocks.
**Why it happens:** Confusion about which thread commands execute on.
**How to avoid:** Commands always execute on the UI/command thread. They communicate with the audio thread via atomics (already established for Transport) or the LockFreeQueue. The CommandDispatcher itself never runs on the audio thread.
**Warning signs:** Audio glitches when executing commands, deadlocks.

### Pitfall 2: Undo/Redo for Transport State
**What goes wrong:** Undoing "play" when the transport is mid-playback creates confusing behavior.
**Why it happens:** Not all operations are meaningfully undoable.
**How to avoid:** Categorize commands into undoable (parameter changes, track edits) and non-undoable (transport play/stop/pause, engine init/shutdown). Non-undoable commands still go through the dispatcher for event emission but bypass the UndoManager.
**Warning signs:** User presses undo and transport unexpectedly stops.

### Pitfall 3: Command Ownership and Memory
**What goes wrong:** Double-free or use-after-free of command objects.
**Why it happens:** `juce::UndoManager::perform()` takes a raw pointer and takes ownership. If you also hold a unique_ptr, you get a double-free.
**How to avoid:** Create commands with `new` and pass raw pointer to `UndoManager::perform()`. For non-undoable commands, use unique_ptr locally and let it destruct. Alternatively, the CommandDispatcher can release ownership before passing to UndoManager.
**Warning signs:** Crashes on undo/redo.

### Pitfall 4: State Synchronization Drift
**What goes wrong:** ProjectState tree drifts out of sync with actual engine state.
**Why it happens:** Some code path modifies engine state without going through the dispatcher.
**How to avoid:** After this phase, ALL state-changing code paths must go through the dispatcher. The existing Phase 2 bridge functions should be refactored to dispatch commands instead of calling Engine methods directly.
**Warning signs:** getProjectState() returns stale data.

### Pitfall 5: Event Emission During Undo
**What goes wrong:** Events during undo confuse the UI (e.g., "BPM changed to 120" when user just wants to undo).
**Why it happens:** Undo triggers the same event as a forward command.
**How to avoid:** Include an `isUndo` flag in emitted events so the UI can differentiate. Or emit specific undo/redo event types.
**Warning signs:** UI shows confusing notifications during undo.

### Pitfall 6: juce_data_structures Module Not Linked
**What goes wrong:** Compilation fails with undefined symbols for UndoManager, UndoableAction.
**Why it happens:** The engine CMakeLists.txt currently links juce_core, juce_audio_*, juce_dsp, but NOT juce_data_structures.
**How to avoid:** Add `juce::juce_data_structures` to target_link_libraries in engine/CMakeLists.txt.
**Warning signs:** Linker errors mentioning UndoManager.

## Code Examples

### juce::UndoManager Usage
```cpp
// Source: JUCE juce_data_structures module
// UndoManager constructor: (int maxNumberOfUnitsToKeep, int minimumTransactionsToKeep)
juce::UndoManager undoManager(0, 200);  // Keep at least 200 transactions

// Performing an action (UndoManager takes ownership of the pointer)
undoManager.perform(new SetBpmCommand(140.0));

// Undo/redo
if (undoManager.canUndo())
    undoManager.undo();

if (undoManager.canRedo())
    undoManager.redo();

// Transaction grouping (multiple actions as one undo step)
undoManager.beginNewTransaction("Set multiple parameters");
undoManager.perform(new SetBpmCommand(140.0));
undoManager.perform(new SetTimeSignatureCommand(3, 4));
// Both undo together on next undo()
```

### juce::JSON Serialization
```cpp
// Source: JUCE juce_core module
// Building JSON with DynamicObject
auto* root = new juce::DynamicObject();
auto* transport = new juce::DynamicObject();
transport->setProperty("state", "stopped");
transport->setProperty("bpm", 120.0);
transport->setProperty("timeSigNumerator", 4);
transport->setProperty("timeSigDenominator", 4);
root->setProperty("transport", juce::var(transport));

// Serialize to string
juce::String json = juce::JSON::toString(juce::var(root));
// {"transport":{"state":"stopped","bpm":120.0,...}}

// Parse from string
auto parsed = juce::JSON::parse(json);
auto bpm = parsed.getProperty("transport", {}).getProperty("bpm", 120.0);
```

### Event Forwarding via Bridge
```cpp
// Persistent ThreadSafeFunction for event emission
Napi::Value SubscribeToEvents(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Expected callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto callback = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(),
        "EventEmitter",
        0,  // unlimited queue
        1   // one reference
    );

    // Store and register with dispatcher
    // ...
    return env.Undefined();
}
```

### IPC Event Pattern (Electron side)
```typescript
// main/index.ts -- forward native events to renderer
import { BrowserWindow } from 'electron'

function forwardEngineEvent(eventName: string, data: unknown) {
    const windows = BrowserWindow.getAllWindows()
    for (const win of windows) {
        win.webContents.send('command:event', { type: eventName, data })
    }
}

// preload/index.ts -- expose event listener
contextBridge.exposeInMainWorld('calliope', {
    // ... existing APIs ...
    onCommandEvent: (callback: (event: { type: string; data: unknown }) => void) =>
        ipcRenderer.on('command:event', (_event, data) => callback(data)),
})
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Direct method calls (Phase 2) | Command dispatch (Phase 3) | Now | All operations go through dispatcher; enables undo, events, AI integration |
| Ad-hoc state queries | JSON state tree | Now | Full project state readable by any consumer |
| No undo | juce::UndoManager | Now | 100+ operation undo history |

## Open Questions

1. **Transaction Grouping Strategy**
   - What we know: juce::UndoManager supports grouping multiple actions into one undo step via `beginNewTransaction()`
   - What's unclear: Which operations should auto-group (e.g., drag-moving multiple notes)
   - Recommendation: For Phase 3, keep each command as its own transaction. Future phases can add grouping for batch operations.

2. **Non-Undoable Command Routing**
   - What we know: Transport play/stop/pause should go through dispatcher for events but not be undoable
   - What's unclear: Best pattern for this split
   - Recommendation: Have `dispatch()` check a `bool isUndoable()` method on the command. If false, execute directly without UndoManager. Still emit events.

3. **State Snapshot vs. Incremental Updates**
   - What we know: AI layer needs to read full project state (ARCH-02)
   - What's unclear: Whether to maintain an always-current state object or rebuild on demand
   - Recommendation: Rebuild on demand from engine state (call getters). Avoids synchronization issues. Only cache if performance becomes a problem.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework (C++) | Catch2 v3.7.1 (already configured) |
| Framework (TS) | Vitest (already configured) |
| Config file (C++) | engine/tests/CMakeLists.txt |
| Config file (TS) | vitest.config.ts |
| Quick run command (C++) | `cd build && ctest --test-dir engine/tests -R "dispatcher" --output-on-failure` |
| Quick run command (TS) | `pnpm test` |
| Full suite command | `cd build && ctest --output-on-failure && cd .. && pnpm test` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ARCH-01 | All operations route through dispatcher | unit (C++) | `ctest -R "dispatcher"` | Wave 0 |
| ARCH-02 | Project state serializes to valid JSON | unit (C++) | `ctest -R "project_state"` | Wave 0 |
| ARCH-05 | Parameters addressable by string ID | unit (C++) | `ctest -R "parameter_registry"` | Wave 0 |
| ARCH-06 | Events emitted for all state changes | unit (C++) | `ctest -R "event_emission"` | Wave 0 |
| PROJ-03 | Undo/redo with 100+ history | unit (C++) | `ctest -R "undo_redo"` | Wave 0 |

### Sampling Rate
- **Per task commit:** `cd build && ctest -R "dispatcher\|project_state\|parameter\|undo" --output-on-failure`
- **Per wave merge:** Full C++ test suite + pnpm test
- **Phase gate:** Full suite green before /gsd:verify-work

### Wave 0 Gaps
- [ ] `engine/tests/test_command_dispatcher.cpp` -- covers ARCH-01, ARCH-06, PROJ-03
- [ ] `engine/tests/test_project_state.cpp` -- covers ARCH-02
- [ ] `engine/tests/test_parameter_registry.cpp` -- covers ARCH-05
- [ ] Add `juce::juce_data_structures` to engine/CMakeLists.txt target_link_libraries
- [ ] Add new test files to engine/tests/CMakeLists.txt

## Project Constraints (from CLAUDE.md)

- **Audio Engine**: C++ with JUCE framework
- **App Shell**: Electron with contextIsolation, no nodeIntegration
- **Native Bridge**: node-addon-api + cmake-js (ThreadSafeFunction + Promise pattern)
- **State Management (renderer)**: Zustand (for future UI consumption of dispatcher events)
- **Build**: CMake for C++, pnpm for JS, electron-vite for dev
- **IPC**: Namespaced channels (engine:*, extend with command:*)
- **Never use**: @electron/remote (synchronous IPC, blocks UI thread)

## Sources

### Primary (HIGH confidence)
- Codebase inspection: engine/include/calliope/*.h, native/src/bridge.cpp, app/src/main/index.ts, app/src/preload/index.ts -- all existing patterns verified by reading source
- JUCE juce_data_structures: UndoManager, UndoableAction API -- from JUCE 8 header files and documentation
- JUCE juce_core: JSON, var, DynamicObject, ListenerList -- from JUCE 8 header files

### Secondary (MEDIUM confidence)
- Command pattern architecture for DAWs -- well-established pattern used in Ardour, Audacity, REAPER internals

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries already in project or part of JUCE
- Architecture: HIGH -- command pattern is textbook, JUCE provides the primitives
- Pitfalls: HIGH -- derived from direct codebase analysis of thread safety patterns and API contracts

**Research date:** 2026-03-28
**Valid until:** 2026-04-28 (stable domain, no external dependencies changing)
