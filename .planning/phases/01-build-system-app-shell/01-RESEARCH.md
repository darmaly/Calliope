# Phase 1: Build System & App Shell - Research

**Researched:** 2026-03-28
**Domain:** Electron + C++/JUCE native addon build system, cmake-js bridge, electron-vite dev workflow
**Confidence:** MEDIUM-HIGH

## Summary

Phase 1 establishes the foundational build system for a hybrid Electron + C++/JUCE application. The core technical challenge is integrating three build worlds: JUCE's CMake-based C++ build, cmake-js's Node.js native addon compilation, and electron-vite's Vite-based Electron bundling. No off-the-shelf template exists for this exact combination -- the CMakeLists.txt that ties JUCE modules to a Node.js native addon via cmake-js is novel integration work requiring careful configuration.

The second challenge is the async bridge pattern. D-08 mandates `Napi::ThreadSafeFunction` from the start, which is the correct architectural choice for a DAW (audio callbacks happen on non-JS threads), but adds complexity to Phase 1's "hello world" bridge. The test tone button (D-09) exercises the full audio path: React UI -> Electron IPC -> main process -> native addon -> JUCE AudioDeviceManager -> speakers.

**Primary recommendation:** Build the CMake integration incrementally -- first get a plain node-addon-api addon compiling via cmake-js, then add JUCE as a subdirectory and link modules, then wire into electron-vite. Do NOT attempt to configure all three build systems simultaneously.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Unified monorepo layout -- top-level folders: `engine/` (C++/JUCE), `native/` (Node addon bridge), `app/` (Electron/React), `vendor/` (JUCE submodule)
- **D-02:** JUCE included as git submodule at `vendor/JUCE`, pinned to v8.0.12
- **D-03:** Single CMake tree -- root `CMakeLists.txt` includes `engine/` as a static lib target, `native/` links against it. One cmake-js invocation builds everything
- **D-04:** Cross-platform from day one -- build system must work on macOS, Linux, and Windows
- **D-05:** Local builds only for Phase 1 -- no CI setup
- **D-06:** Electron + React dev server uses electron-vite with HMR for the renderer
- **D-07:** First round-trip returns JUCE version string and available audio devices
- **D-08:** Async from the start -- use `Napi::ThreadSafeFunction` pattern even for Phase 1 queries
- **D-09:** UI includes a test tone button (simple sine wave via JUCE audio output) as proof-of-life

### Claude's Discretion
- Native C++ rebuild strategy (manual command vs file watcher with auto-rebuild)
- Package manager choice (pnpm vs npm)
- Where the native addon loads (main process vs utility process)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ARCH-03 | Native bridge (node-addon-api) connects C++ audio engine to Electron/Node.js layer | cmake-js + node-addon-api CMake integration pattern, ThreadSafeFunction async pattern, Electron native module loading |
| UI-01 | Electron application shell with React-based UI | electron-vite project scaffolding, React 19 + TypeScript renderer, Electron 35 LTS shell |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **Audio Engine**: C++ with JUCE framework (v8.0.12)
- **App Shell**: Electron (35.x LTS)
- **Native Bridge**: node-addon-api (8.x) + cmake-js (8.0.0)
- **Frontend**: React 19 + TypeScript 5.7+ + Vite 6.x via electron-vite 3.x
- **Package Manager**: pnpm 10.x recommended (CLAUDE.md), but discretionary per CONTEXT.md
- **Build System**: CMake 3.24+ for C++, cmake-js bridges into npm
- **No Projucer** -- JUCE 8 uses CMake as primary build system
- **No node-gyp** -- cmake-js is the required alternative
- **No @electron/remote** -- synchronous IPC forbidden
- **Avoid NAN** -- legacy, use node-addon-api/N-API instead

## Standard Stack

### Core (Phase 1 scope only)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Electron | 35.7.5 | Desktop app shell | LTS line with Node 22 support, per CLAUDE.md |
| React | 19.2.4 | UI framework | Locked in CLAUDE.md |
| TypeScript | 5.7+ | Type safety | Locked in CLAUDE.md (5.7+ required, latest is 6.0.2) |
| electron-vite | 3.1.0 | Dev server + build | CLAUDE.md specifies 3.x; supports Vite 6; latest 3.x is 3.1.0 |
| Vite | 6.x | Bundling (via electron-vite) | CLAUDE.md specifies 6.x (latest 6.x available); electron-vite 3.x compatible |
| node-addon-api | 8.7.0 | C++ N-API wrapper | CLAUDE.md specifies 8.x (latest is 8.7.0) |
| cmake-js | 8.0.0 | Native addon build tool | CLAUDE.md locks to 8.0.0 |
| JUCE | 8.0.12 | Audio engine framework | Locked in D-02, git submodule |
| CMake | 3.24+ | C++ build system | Required by JUCE 8 and cmake-js |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| electron-builder | 25.1.8 | App packaging | Final build step to produce macOS app |
| Tailwind CSS | 4.x | UI styling | For the test tone button and basic shell UI |
| Zustand | 5.0.12 | State management | Minimal use in Phase 1 (transport state for test tone) |
| @swc/core | ^1.0.0 | Fast transpilation | Peer dependency of electron-vite 3.x |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| electron-vite 3.x | electron-vite 5.0 | v5 just released, drops Vite 4 support. CLAUDE.md pins 3.x which is stable and supports Vite 6. Stick with 3.x |
| pnpm | npm | npm 11.6.2 already installed. pnpm recommended in CLAUDE.md but discretionary. **Recommend pnpm** -- strict deps prevent phantom dependency issues critical for native addon projects |
| Main process addon loading | Utility process | Main process is simpler, proven pattern. Utility process isolates crashes but adds IPC complexity. **Recommend main process** for Phase 1, can migrate later |

**Installation (Phase 1 packages):**
```bash
# Install pnpm first
npm install -g pnpm@10

# Initialize project
pnpm init

# Core Electron + React
pnpm add -D electron@35.7.5 electron-vite@3.1.0 electron-builder@25.1.8 vite@6
pnpm add react@19 react-dom@19 @types/react @types/react-dom

# TypeScript
pnpm add -D typescript@5.7

# State (minimal use in Phase 1)
pnpm add zustand@5

# Styling
pnpm add -D tailwindcss@4

# Native addon build
pnpm add -D cmake-js@8.0.0 node-addon-api@8.7.0

# SWC (peer dep of electron-vite)
pnpm add -D @swc/core
```

**System dependencies (macOS):**
```bash
brew install cmake
```

## Architecture Patterns

### Recommended Project Structure (from D-01)

```
calliope/
├── app/                          # Electron + React application
│   ├── src/
│   │   ├── main/                 # Electron main process
│   │   │   ├── index.ts          # App entry, window creation, IPC handlers
│   │   │   └── native-bridge.ts  # Typed wrapper around native addon
│   │   ├── preload/              # Preload scripts (context bridge)
│   │   │   └── index.ts          # Exposes safe IPC channels to renderer
│   │   └── renderer/             # React UI
│   │       ├── index.html
│   │       ├── main.tsx          # React entry point
│   │       ├── App.tsx           # Root component
│   │       └── components/
│   │           └── TestTone.tsx  # Test tone button component
│   ├── electron.vite.config.ts   # electron-vite configuration
│   └── tsconfig.json
├── engine/                       # C++ audio engine (JUCE-based)
│   ├── CMakeLists.txt            # Engine static library target
│   ├── include/
│   │   └── calliope/
│   │       └── engine.h          # Public API header
│   └── src/
│       ├── engine.cpp            # AudioDeviceManager, device enumeration
│       └── test_tone.cpp         # Simple sine wave generator
├── native/                       # Node.js native addon (bridge)
│   ├── CMakeLists.txt            # Addon target, links engine
│   └── src/
│       ├── addon.cpp             # Napi::Addon registration
│       ├── bridge.cpp            # Async methods using ThreadSafeFunction
│       └── bridge.h
├── vendor/                       # External dependencies
│   └── JUCE/                     # Git submodule (v8.0.12)
├── CMakeLists.txt                # Root CMake -- includes engine/ and native/
├── package.json                  # npm scripts, cmake-js config
├── pnpm-workspace.yaml           # If using pnpm workspaces (optional)
└── electron-builder.yml          # Packaging configuration
```

### Pattern 1: Single CMake Tree (D-03)

**What:** Root CMakeLists.txt orchestrates the entire C++ build. cmake-js invokes CMake at root, which includes engine/ as a static library and native/ as the .node addon target.

**When to use:** Always -- this is a locked decision.

**Example:**
```cmake
# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.24)
project(calliope-native)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE (git submodule)
add_subdirectory(vendor/JUCE)

# Engine static library (links JUCE modules)
add_subdirectory(engine)

# Node.js native addon (links engine)
add_subdirectory(native)
```

```cmake
# engine/CMakeLists.txt
add_library(calliope_engine STATIC
    src/engine.cpp
    src/test_tone.cpp
)

target_include_directories(calliope_engine PUBLIC include)

# Link JUCE modules needed for Phase 1
target_link_libraries(calliope_engine
    PUBLIC
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_events
        juce::juce_dsp
)

# JUCE requires these compile definitions on the target
target_compile_definitions(calliope_engine
    PUBLIC
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
```

```cmake
# native/CMakeLists.txt
# cmake-js provides CMAKE_JS_INC, CMAKE_JS_LIB, CMAKE_JS_SRC
include_directories(${CMAKE_JS_INC})

# Node-API version (9 = latest stable)
add_definitions(-DNAPI_VERSION=9)

add_library(calliope_addon SHARED
    src/addon.cpp
    src/bridge.cpp
    ${CMAKE_JS_SRC}  # Required for Windows delay-load hook
)

# Set .node extension
set_target_properties(calliope_addon PROPERTIES
    PREFIX ""
    SUFFIX ".node"
)

# Link against engine and node libraries
target_link_libraries(calliope_addon
    PRIVATE
        calliope_engine
        ${CMAKE_JS_LIB}
)

# node-addon-api headers (resolved via cmake-js)
execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(calliope_addon PRIVATE ${NODE_ADDON_API_DIR})
```

### Pattern 2: Async Native Bridge with ThreadSafeFunction (D-08)

**What:** All native addon methods use async patterns from day one. Even simple queries like "get JUCE version" use promises on the JS side.

**When to use:** Always -- locked decision to establish the async pattern early.

**Example:**
```cpp
// native/src/bridge.cpp
#include <napi.h>
#include "calliope/engine.h"

// Async wrapper for getting engine info
Napi::Value GetEngineInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Create a deferred promise
    auto deferred = Napi::Promise::Deferred::New(env);

    // Create ThreadSafeFunction for callback from any thread
    auto tsfn = Napi::ThreadSafeFunction::New(
        env,
        Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetEngineInfo",
        0,  // unlimited queue
        1   // initial thread count
    );

    // For simple queries, we can resolve immediately on a worker
    // but using TSFN establishes the pattern
    std::thread([deferred, tsfn]() {
        auto juceVersion = calliope::Engine::getJuceVersion();
        auto devices = calliope::Engine::getAudioDevices();

        tsfn.BlockingCall([deferred, juceVersion, devices](
            Napi::Env env, Napi::Function) {
            auto result = Napi::Object::New(env);
            result.Set("juceVersion", Napi::String::New(env, juceVersion));

            auto deviceArray = Napi::Array::New(env, devices.size());
            for (size_t i = 0; i < devices.size(); i++) {
                deviceArray.Set(i, Napi::String::New(env, devices[i]));
            }
            result.Set("audioDevices", deviceArray);

            deferred.Resolve(result);
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}
```

### Pattern 3: Electron IPC with Context Isolation

**What:** Renderer cannot access Node.js APIs directly. Preload script exposes typed IPC channels via contextBridge.

**When to use:** Always -- Electron security best practice.

**Example:**
```typescript
// app/src/preload/index.ts
import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('calliope', {
  getEngineInfo: () => ipcRenderer.invoke('engine:getInfo'),
  startTestTone: (frequency: number) => ipcRenderer.invoke('engine:startTestTone', frequency),
  stopTestTone: () => ipcRenderer.invoke('engine:stopTestTone'),
})
```

```typescript
// app/src/main/index.ts
import { app, BrowserWindow, ipcMain } from 'electron'
import { createRequire } from 'module'

const require = createRequire(import.meta.url)

// Load native addon from build output
const native = require('../../build/Release/calliope_addon.node')

ipcMain.handle('engine:getInfo', async () => {
  return await native.getEngineInfo()
})

ipcMain.handle('engine:startTestTone', async (_event, frequency: number) => {
  return await native.startTestTone(frequency)
})

ipcMain.handle('engine:stopTestTone', async () => {
  return await native.stopTestTone()
})
```

### Pattern 4: electron-vite Configuration

**What:** Unified Vite config for main/preload/renderer with native addon as external.

**Example:**
```typescript
// app/electron.vite.config.ts
import { defineConfig, externalizeDepsPlugin } from 'electron-vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  main: {
    plugins: [externalizeDepsPlugin()],
    build: {
      rollupOptions: {
        external: [/\.node$/]  // Don't bundle .node files
      }
    }
  },
  preload: {
    plugins: [externalizeDepsPlugin()]
  },
  renderer: {
    plugins: [react()],
  }
})
```

### Anti-Patterns to Avoid

- **Synchronous IPC (`@electron/remote`, `ipcRenderer.sendSync`):** Blocks the UI thread. Always use `ipcRenderer.invoke` + `ipcMain.handle` for request/response.
- **Loading native addon in renderer process:** Security risk and crashes the UI. Always load in main process.
- **Using node-gyp or Projucer:** Both are deprecated/legacy for this stack.
- **Compiling JUCE modules separately then linking:** JUCE modules have interdependencies and shared compile definitions. Let JUCE's CMake handle module compilation via `target_link_libraries`.
- **Bundling the .node file with Vite:** Native addons must be loaded via `require()` at runtime, not bundled. Mark as external in Vite config.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Audio device enumeration | Custom platform APIs (CoreAudio/WASAPI/ALSA) | JUCE AudioDeviceManager | Handles all platforms, device hot-plugging, format negotiation |
| Sine wave generation | Manual sample buffer math | JUCE dsp::Oscillator | Handles anti-aliasing, phase continuity, sample-rate-correct |
| Node.js C++ bridge | Raw N-API C calls | node-addon-api C++ wrappers | Type-safe, exception handling, cleaner API |
| Electron build tooling | Custom webpack/rollup for Electron | electron-vite | Handles main/preload/renderer separation, HMR, production builds |
| Cross-thread JS callbacks | Manual libuv integration | Napi::ThreadSafeFunction | Handles queueing, GC prevent, reference counting |

## Common Pitfalls

### Pitfall 1: cmake-js Not Finding Electron Headers
**What goes wrong:** cmake-js builds against system Node.js headers instead of Electron's, causing ABI mismatch and crash on load.
**Why it happens:** Missing `cmake-js` config in package.json or wrong `runtimeVersion`.
**How to avoid:** Add to root package.json:
```json
{
  "cmake-js": {
    "runtime": "electron",
    "runtimeVersion": "35.7.5",
    "arch": "x64"
  }
}
```
**Warning signs:** Addon loads in plain Node.js but crashes in Electron, or `process.versions.modules` mismatch errors.

### Pitfall 2: JUCE GUI Requirements on Headless Targets
**What goes wrong:** JUCE modules like `juce_gui_basics` pull in platform GUI dependencies (Cocoa, X11, etc.) even when you only want audio.
**Why it happens:** JUCE's module dependency graph -- `juce_audio_devices` depends on `juce_events` which may pull in GUI-related event loops.
**How to avoid:** Use `JUCE_STANDALONE_APPLICATION=0` compile definition. Only link the modules actually needed. For Phase 1: `juce_core`, `juce_audio_basics`, `juce_audio_devices`, `juce_events`, `juce_dsp`. Avoid `juce_gui_basics` and `juce_audio_utils` (the latter depends on GUI).
**Warning signs:** Link errors about Cocoa/NSApplication on macOS, X11 on Linux.

### Pitfall 3: JUCE Message Thread Not Running
**What goes wrong:** `AudioDeviceManager` operations fail or hang because JUCE expects a message loop.
**Why it happens:** JUCE uses a message thread for callbacks. In a native addon context, there's no JUCE application event loop.
**How to avoid:** Initialize JUCE's message manager explicitly: `juce::MessageManager::getInstance()` in the addon init. For the test tone, ensure audio callbacks run on the correct thread.
**Warning signs:** Assertions in debug builds about "message thread" or "not on the message thread".

### Pitfall 4: Native Addon Path Resolution in Production
**What goes wrong:** `require('./build/Release/addon.node')` works in dev but breaks after electron-builder packages the app.
**Why it happens:** electron-builder moves files to asar archive or different directory structure.
**How to avoid:** Use `__dirname` relative paths and configure electron-builder to include the .node file as an unpacked resource:
```yaml
# electron-builder.yml
asarUnpack:
  - "**/*.node"
```
**Warning signs:** "Cannot find module" errors only in packaged builds.

### Pitfall 5: Vite Trying to Bundle Native Addon
**What goes wrong:** Vite throws errors trying to process `.node` binary files or `require()` calls.
**Why it happens:** Vite transforms all imports by default.
**How to avoid:** Mark native addon as external in electron-vite config. Use `externalizeDepsPlugin()` for the main process config.
**Warning signs:** Build errors about binary files, or "require is not defined" in bundled output.

### Pitfall 6: POSITION_INDEPENDENT_CODE Not Set
**What goes wrong:** Linking fails when building the engine static library on Linux.
**Why it happens:** Shared libraries (.node is a shared lib) require PIC code. Static libs linked into them must also be PIC.
**How to avoid:** Set `CMAKE_POSITION_INDEPENDENT_CODE ON` at the root CMakeLists.txt level.
**Warning signs:** Linker errors about "relocation" on Linux only.

## Code Examples

### JUCE Engine -- Device Enumeration and Test Tone

```cpp
// engine/include/calliope/engine.h
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace calliope {

class Engine {
public:
    static std::string getJuceVersion();
    static std::vector<std::string> getAudioDevices();

    // Test tone control
    static bool startTestTone(double frequency, int deviceIndex = -1);
    static void stopTestTone();

private:
    Engine() = default;
};

} // namespace calliope
```

```cpp
// engine/src/engine.cpp
#include "calliope/engine.h"
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace calliope {

std::string Engine::getJuceVersion() {
    return juce::SystemStats::getJUCEVersion().toStdString();
}

std::vector<std::string> Engine::getAudioDevices() {
    // Ensure message manager exists
    juce::MessageManager::getInstance();

    juce::AudioDeviceManager manager;
    std::vector<std::string> devices;

    for (auto* type : manager.getAvailableDeviceTypes()) {
        type->scanForDevices();
        for (auto& name : type->getDeviceNames()) {
            devices.push_back(type->getTypeName().toStdString()
                + ": " + name.toStdString());
        }
    }

    return devices;
}

} // namespace calliope
```

### Native Addon Registration

```cpp
// native/src/addon.cpp
#include <napi.h>

// Forward declarations from bridge.cpp
Napi::Value GetEngineInfo(const Napi::CallbackInfo& info);
Napi::Value StartTestTone(const Napi::CallbackInfo& info);
Napi::Value StopTestTone(const Napi::CallbackInfo& info);

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("getEngineInfo",
        Napi::Function::New(env, GetEngineInfo));
    exports.Set("startTestTone",
        Napi::Function::New(env, StartTestTone));
    exports.Set("stopTestTone",
        Napi::Function::New(env, StopTestTone));
    return exports;
}

NODE_API_MODULE(calliope_addon, Init)
```

### React Test Tone Component

```tsx
// app/src/renderer/components/TestTone.tsx
import { useState } from 'react'

export function TestTone() {
  const [playing, setPlaying] = useState(false)
  const [frequency, setFrequency] = useState(440)

  const toggle = async () => {
    if (playing) {
      await window.calliope.stopTestTone()
    } else {
      await window.calliope.startTestTone(frequency)
    }
    setPlaying(!playing)
  }

  return (
    <div>
      <h2>Test Tone</h2>
      <label>
        Frequency: {frequency} Hz
        <input
          type="range" min={100} max={2000} value={frequency}
          onChange={(e) => setFrequency(Number(e.target.value))}
        />
      </label>
      <button onClick={toggle}>
        {playing ? 'Stop' : 'Play'} Test Tone
      </button>
    </div>
  )
}
```

### TypeScript Type Declarations for Preload API

```typescript
// app/src/renderer/types/calliope.d.ts
interface CalliopeAPI {
  getEngineInfo(): Promise<{
    juceVersion: string
    audioDevices: string[]
  }>
  startTestTone(frequency: number): Promise<void>
  stopTestTone(): Promise<void>
}

declare global {
  interface Window {
    calliope: CalliopeAPI
  }
}

export {}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Projucer for JUCE builds | CMake (juce_add_*) | JUCE 6+ (2021) | Projucer deprecated for new projects |
| node-gyp for native addons | cmake-js | 2020+ | Better CMake integration, Electron support |
| NAN for native abstractions | node-addon-api (N-API) | 2018+ | ABI stable, no recompilation per Node version |
| Electron Forge + Webpack | electron-vite | 2023+ | Faster HMR, simpler config, Vite ecosystem |
| electron-vite 2.x | electron-vite 3.x (Vite 6) | 2025 | Vite 6 support, latest is 3.1.0 |
| Zustand 4.x | Zustand 5.x | 2024 | API changes; latest is 5.0.12 |

**Note on version drift from CLAUDE.md:**
- electron-builder: CLAUDE.md says 25.x, latest 25.x is 25.1.8 (was 25.x range)
- Zustand: CLAUDE.md says 4.5.x but latest is 5.0.12. Use 5.x as it is current
- node-addon-api: CLAUDE.md says 8.5.0 but latest is 8.7.0. Use 8.7.0

## Open Questions

1. **JUCE MessageManager in addon context**
   - What we know: JUCE expects a message thread for AudioDeviceManager callbacks. In a native addon, there is no JUCE application loop.
   - What's unclear: Whether `MessageManager::getInstance()` alone is sufficient or if we need `initialiseJuce_GUI()` or a running message loop thread.
   - Recommendation: Spike this during implementation. Try `MessageManager::getInstance()` first. If audio callbacks fail, create a dedicated JUCE message thread in the addon initialization.

2. **electron-vite and native addon rebuild workflow**
   - What we know: electron-vite handles HMR for TypeScript/React. cmake-js rebuilds the native addon separately.
   - What's unclear: Best DX for iterating on C++ code -- manual `npm run build:native` vs a file watcher.
   - Recommendation (Claude's discretion): Manual rebuild command. C++ compile times make file-watcher rebuilds annoying (false triggers, long builds). Use an npm script: `"build:native": "cmake-js build"`.

3. **Loading the .node addon in electron-vite dev mode**
   - What we know: The built .node file lives in `build/Release/`. electron-vite's main process runs from `out/main/`.
   - What's unclear: Exact path resolution between dev and production modes.
   - Recommendation: Use `app.isPackaged` to switch paths, or use a consistent path relative to the project root in dev.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Node.js | Everything | Yes | v20.19.5 | -- |
| npm | Package install | Yes | 11.6.2 | -- |
| pnpm | Package manager (recommended) | No | -- | Install via `npm install -g pnpm@10` |
| CMake | C++ build system | **No** | -- | **Install via `brew install cmake`** |
| C++ compiler (clang) | Native addon compilation | Yes | Apple clang 17.0.0 | -- |
| Git | Submodule management | Yes | 2.52.0 | -- |
| Xcode CLI Tools | macOS build headers | Yes | /Library/Developer/CommandLineTools | -- |
| pkg-config | Library discovery | No | -- | Not strictly required; CMake find_package handles JUCE |
| Python 3 | node-gyp fallback (unused) | Yes | 3.9.6 | -- |
| Make | CMake build backend | Yes | GNU Make 3.81 | -- |

**Missing dependencies with no fallback:**
- **CMake** -- absolutely required. Must install before any C++ work: `brew install cmake`

**Missing dependencies with fallback:**
- **pnpm** -- recommended but npm works. Install with `npm install -g pnpm@10`

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Vitest 4.x (TypeScript), Catch2 (C++) |
| Config file | None -- Wave 0 must create `vitest.config.ts` |
| Quick run command | `npx vitest run --reporter=verbose` |
| Full suite command | `npx vitest run && ctest --test-dir build` |

### Phase Requirements -> Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ARCH-03 | Native addon loads and returns data from C++ | integration | `npx vitest run tests/bridge.test.ts -t "native bridge"` | No -- Wave 0 |
| ARCH-03 | C++ engine compiles and links JUCE | unit (C++) | `ctest --test-dir build -R engine_test` | No -- Wave 0 |
| UI-01 | Electron app launches with React window | smoke | Manual -- launch app, verify React renders | N/A (manual) |
| UI-01 | Test tone button triggers audio output | smoke | Manual -- click button, verify sound | N/A (manual) |

### Sampling Rate

- **Per task commit:** `npx vitest run` (TypeScript tests only -- fast)
- **Per wave merge:** `npx vitest run && ctest --test-dir build` (full suite)
- **Phase gate:** Full suite green + manual smoke test (app launches, test tone plays)

### Wave 0 Gaps

- [ ] `vitest.config.ts` -- Vitest configuration at project root or app/ level
- [ ] `tests/bridge.test.ts` -- Tests native addon loading and round-trip calls
- [ ] `engine/tests/engine_test.cpp` -- Catch2 test for JUCE version and device enumeration
- [ ] `engine/tests/CMakeLists.txt` -- Catch2 test target in CMake
- [ ] Framework install: `pnpm add -D vitest` and add Catch2 to vendor/ or FetchContent

**Note on testing native addons with Vitest:** Testing native addons requires building them first. Vitest tests that import the .node file will need the addon pre-built. For CI-less Phase 1, this means `npm run build:native && npx vitest run`. Also, Vitest runs in Node.js, not Electron -- mock `electron` imports or test bridge logic in isolation.

## Sources

### Primary (HIGH confidence)
- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md) -- CMake functions, module linking, target creation
- [node-addon-api cmake-js docs](https://github.com/nodejs/node-addon-api/blob/main/doc/cmake-js.md) -- CMake integration with node-addon-api
- [cmake-js GitHub](https://github.com/cmake-js/cmake-js) -- Electron support, CMAKE_JS_* variables, package.json config
- [Electron native modules docs](https://www.electronjs.org/docs/latest/tutorial/using-native-node-modules) -- Building native modules for Electron
- [electron-vite Getting Started](https://electron-vite.org/guide/) -- Project structure, configuration
- [node-addon-api ThreadSafeFunction](https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe_function.md) -- Async callback pattern
- npm registry version checks (2026-03-28) -- All package versions verified against registry

### Secondary (MEDIUM confidence)
- [JUCE forum: static library with CMake](https://forum.juce.com/t/how-to-create-a-static-library-that-use-juce-in-official-cmake/50080) -- JUCE static lib pattern (community, not official)
- [Melatonin: How to use CMake with JUCE](https://melatonin.dev/blog/how-to-use-cmake-with-juce/) -- Practical CMake patterns

### Tertiary (LOW confidence)
- JUCE MessageManager behavior in addon context -- no authoritative source found for this specific use case. Flagged as Open Question #1.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all versions verified against npm registry and official docs
- Architecture: MEDIUM-HIGH -- project structure and CMake patterns follow official docs, but JUCE-in-addon is novel integration
- Pitfalls: MEDIUM -- based on multiple sources and known patterns, but JUCE MessageManager behavior in addon context unverified
- Build system: MEDIUM -- cmake-js + JUCE combination lacks authoritative examples; each piece documented but integration is uncharted

**Research date:** 2026-03-28
**Valid until:** 2026-04-28 (30 days -- stable technologies, versions pinned)
