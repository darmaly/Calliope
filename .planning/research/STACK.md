# Technology Stack

**Project:** LuneyTunes (AI-Powered DAW)
**Researched:** 2026-03-27
**Overall Confidence:** HIGH

---

## Recommended Stack

### Audio Engine (C++)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| JUCE | 8.0.12 | Audio engine framework | Industry standard for DAW/plugin development. Provides DSP, MIDI, audio I/O, synthesis primitives, and cross-platform audio device management. No viable alternative at this maturity level. | HIGH |
| CMake | 3.24+ | C++ build system | JUCE 8 uses CMake as its primary build system (Projucer is deprecated for new projects). Required for cmake-js integration with Node.js. | HIGH |
| libsndfile | 1.2.2 | Audio file I/O (WAV, AIFF, FLAC, OGG) | C library with LGPL license. Reads/writes WAV, AIFF, FLAC, OGG/Vorbis through a single API. JUCE has built-in format readers too, but libsndfile handles edge cases better for import/export. | MEDIUM |
| LAME | 3.100 | MP3 encoding | The standard MP3 encoder. LGPL licensed. Needed for MP3 export requirement. JUCE does not include MP3 encoding (patent legacy). | HIGH |
| libsamplerate | 0.2.2 | Sample rate conversion | 145dB SNR, arbitrary conversion ratios. Needed when importing audio at different sample rates than project rate. | MEDIUM |

#### JUCE Modules to Use

| Module | Purpose | Notes |
|--------|---------|-------|
| `juce_audio_basics` | Audio buffer types, MIDI messages | Foundation -- always needed |
| `juce_audio_devices` | Audio hardware I/O, MIDI device enumeration | Manages CoreAudio/WASAPI/ALSA backends |
| `juce_audio_formats` | Read/write WAV, AIFF, FLAC, OGG | Built-in format support; supplement with libsndfile for MP3 import |
| `juce_audio_processors` | AudioProcessor base class, parameter system | Core abstraction for synths and effects |
| `juce_audio_utils` | Audio device selector, MIDI keyboard component | UI helpers if needed on the C++ side |
| `juce_dsp` | DSP primitives: oscillators, filters, FFT, convolution | `dsp::ProcessorChain`, `dsp::LadderFilter`, `dsp::Reverb`, `dsp::Compressor` |
| `juce_midi_ci` | MIDI 2.0 capability inquiry | Future-proofing for MIDI 2.0 devices |
| `juce_core` | Threading, JSON, file I/O, memory management | Foundation utility module |
| `juce_events` | Message loop, timers, async callbacks | Required for non-UI event handling in the engine |

**Do NOT use:** `juce_gui_basics`, `juce_gui_extra`, `juce_opengl` -- the UI is in React/Electron, not JUCE. Including GUI modules adds unnecessary weight and complexity.

---

### Native Bridge (C++ <-> Node.js)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| node-addon-api | 8.5.0 | C++ wrapper for Node-API | Official C++ wrapper over the C-level Node-API (N-API). Type-safe, exception-handling, forward/backward compatible across Node versions without recompilation. This is the official recommendation from Node.js core team. | HIGH |
| cmake-js | 8.0.0 | Build native addons with CMake | Since JUCE already uses CMake, cmake-js lets you build the native addon with the same build system. Out-of-the-box Electron support with no post-build steps. Far superior to node-gyp for CMake-based projects. | HIGH |

**Do NOT use:**
- **NAN (Native Abstractions for Node.js)** -- Legacy. Requires recompilation for each Node.js version. node-addon-api/N-API is the official successor.
- **node-gyp directly** -- GYP is a dead build system (Google abandoned it). cmake-js is the modern alternative and aligns with JUCE's CMake build.
- **napi-rs (Rust)** -- Adds Rust to the stack when the engine is already C++. Unnecessary complexity.
- **Electron's `@electron/remote`** -- Synchronous IPC, blocks UI thread. Never use for audio engine communication.

#### IPC Architecture

The native addon exposes the C++ audio engine as a Node.js module loaded in Electron's main process. Communication pattern:

```
Renderer (React UI)
    |-- Electron IPC (ipcRenderer.invoke) -->
Main Process (Node.js)
    |-- Direct function call -->
Native Addon (node-addon-api)
    |-- C++ method call -->
JUCE Audio Engine
```

**For real-time data (waveforms, meters):** Use `SharedArrayBuffer` passed via IPC to avoid serialization overhead. The renderer reads directly from shared memory that the audio engine writes to. This eliminates the IPC bottleneck for high-frequency visual updates.

**For commands (load instrument, set parameter):** Use standard `ipcRenderer.invoke` / `ipcMain.handle` async pattern. Commands are low-frequency and can tolerate serialization.

---

### Application Shell

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Electron | 35.x (LTS) | Desktop app shell | Cross-platform (macOS/Win/Linux), Node.js integration for native addons, proven for complex apps (VS Code, Figma). Use v35 for Node 22 LTS support rather than bleeding-edge v38. | HIGH |
| electron-vite | 3.x | Build tooling | Lighter than Electron Forge for Vite-based projects. Follows Vite's project structure naturally. Electron Forge's Vite plugin is still marked experimental (v7.5.0+). electron-vite is battle-tested and has higher weekly downloads (263K vs 2K). Use electron-vite for dev, and electron-builder for packaging. | MEDIUM |
| electron-builder | 25.x | Packaging and distribution | Produces DMG (macOS), NSIS/MSI (Windows), AppImage/deb (Linux). More flexible than Forge's makers for custom native addon bundling. | HIGH |

**Do NOT use:**
- **Electron Forge with Vite** -- Vite support is officially "experimental" as of v7.5.0. The electron-vite ecosystem is more mature for Vite-based projects.
- **Tauri** -- Rust-based, no Node.js runtime = no native addon bridge to C++/JUCE. Wrong architecture for this project.
- **Neutralinojs** -- Too immature for a complex DAW application.

---

### Frontend (React UI)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| React | 19.x | UI framework | Already decided. React 19 for concurrent features and improved rendering performance. | HIGH |
| TypeScript | 5.7+ | Type safety | Non-negotiable for a project this complex. Catches bridge API mismatches at compile time. | HIGH |
| Zustand | 4.5.x | Global state management | Minimal API, hook-first, handles complex subscriptions cleanly. Perfect for DAW state (tracks, mixer, transport). Selector-based subscriptions prevent unnecessary re-renders when one track changes. Far simpler than Redux for this use case. | HIGH |
| PixiJS | 8.x | Canvas/WebGL rendering for piano roll, waveforms, timeline | GPU-accelerated 2D rendering via WebGL (with WebGPU option). React integration via `@pixi/react`. A DAW's visual components (piano roll grid, waveform display, automation lanes) need canvas/WebGL performance -- DOM rendering will not keep up with thousands of MIDI notes or real-time waveform updates. | HIGH |
| wavesurfer.js | 7.x | Waveform visualization | Mature library for audio waveform display with zoom, regions, timeline. Use for arrangement view waveform clips. Can render from pre-computed peak data (generated by C++ engine) rather than decoding audio in the browser. | MEDIUM |
| @pixi/react | 8.x | React bindings for PixiJS | Official React 19 + PixiJS 8 integration. Lets you write PixiJS scenes as React components while PixiJS handles the actual rendering. | MEDIUM |
| Tailwind CSS | 4.x | Utility-first styling | For non-canvas UI (toolbar, sidebar, dialogs, chat interface). Fast iteration, consistent design system. DAW chrome (non-canvas parts) benefits from rapid prototyping. | MEDIUM |

#### UI Component Strategy

**Canvas-rendered (PixiJS):**
- Piano roll (MIDI note grid, note editing)
- Waveform display (audio clips on timeline)
- Automation lanes
- Mixer level meters (real-time VU/peak)
- Timeline ruler and playhead

**DOM-rendered (React + Tailwind):**
- Transport controls (play/stop/record)
- Track headers (name, mute/solo/arm)
- Mixer channel strips (faders as custom React components)
- AI chat interface
- File browser / preset browser
- Settings / preferences dialogs
- Toolbar and menus

**Rationale:** Canvas for anything with hundreds/thousands of visual elements or real-time animation. DOM for standard UI controls where accessibility and native behavior matter.

**Do NOT use:**
- **react-piano-roll (npm)** -- Unmaintained, low download count, just a PixiJS fork without audio integration. Build custom on PixiJS.
- **Reactronica** -- Uses Tone.js/Web Audio API for audio. We have a C++ engine; Reactronica's audio layer is redundant and would conflict.
- **Material UI / Ant Design** -- Too opinionated for a DAW. DAW interfaces have unique interaction patterns (right-click menus, drag-to-select, custom scrolling) that fight against component library conventions.
- **Redux** -- Overkill boilerplate. Zustand does everything needed with 1/10th the code.

---

### AI Integration Layer

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| AI SDK (Vercel) | 6.x | Multi-provider AI abstraction | Unified API for Claude, GPT, Gemini, etc. Switch providers with one line. Built-in streaming, tool calling, structured output. The `ai` npm package has massive adoption. Perfect for BYOK -- user provides API key, SDK handles the rest. | HIGH |
| @anthropic-ai/sdk | latest | Direct Anthropic SDK (fallback) | If AI SDK doesn't expose a Claude-specific feature (e.g., extended thinking, computer use), fall back to direct SDK. 7M+ weekly downloads, production-grade. | MEDIUM |
| @ai-sdk/anthropic | latest | AI SDK Anthropic provider | Provider adapter for AI SDK to talk to Claude. Install alongside `ai` core package. | HIGH |
| @ai-sdk/openai | latest | AI SDK OpenAI provider | Provider adapter for GPT models. BYOK means supporting multiple providers. | HIGH |
| zod | 3.x | Schema validation | Define structured output schemas for AI tool calls. AI SDK uses Zod natively for `generateObject` / `streamObject`. Also validates DAW command schemas from AI responses. | HIGH |

#### AI Architecture Pattern

```typescript
// BYOK: User provides their API key, we create the provider
import { createAnthropic } from '@ai-sdk/anthropic';
import { generateText, tool } from 'ai';
import { z } from 'zod';

const anthropic = createAnthropic({ apiKey: userProvidedKey });

// AI has tools that map to DAW commands
const result = await generateText({
  model: anthropic('claude-sonnet-4-20250514'),
  system: dawSystemPrompt,      // Describes available instruments, effects, current project state
  messages: conversationHistory,
  tools: {
    addTrack: tool({
      description: 'Add a new track to the project',
      parameters: z.object({
        name: z.string(),
        type: z.enum(['audio', 'midi', 'bus']),
        instrument: z.string().optional(),
      }),
      execute: async (params) => dawEngine.addTrack(params),
    }),
    writeMidiNotes: tool({
      description: 'Write MIDI notes to a track',
      parameters: z.object({
        trackId: z.string(),
        notes: z.array(z.object({
          pitch: z.number().min(0).max(127),
          velocity: z.number().min(0).max(127),
          startBeat: z.number(),
          durationBeats: z.number(),
        })),
      }),
      execute: async (params) => dawEngine.writeMidi(params),
    }),
    // ... more DAW command tools
  },
});
```

**Do NOT use:**
- **LangChain** -- Over-abstracted for this use case. AI SDK is simpler, more performant, and has better TypeScript support. LangChain adds dependency weight and complexity for features we don't need (RAG, vector stores, document loaders).
- **Direct HTTP fetch to AI APIs** -- Loses streaming, retry logic, type safety. SDK exists for a reason.
- **Running AI models locally** -- Out of scope. BYOK means cloud APIs. Local models can be added later if users want.

---

### Audio Format Support

| Format | Read | Write | Library | Notes |
|--------|------|-------|---------|-------|
| WAV | Yes | Yes | JUCE `juce_audio_formats` | Native support, 16/24/32-bit, PCM and float |
| AIFF | Yes | Yes | JUCE `juce_audio_formats` | Native support |
| FLAC | Yes | Yes | JUCE `juce_audio_formats` + libFLAC | JUCE has FLAC support via bundled codec |
| OGG Vorbis | Yes | Yes | JUCE `juce_audio_formats` | Native support |
| MP3 | Yes | Yes | JUCE (read) + LAME (write) | JUCE can read MP3. Encoding requires LAME (LGPL). Bundle as dynamic library. |

---

### Build Tooling

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| CMake | 3.24+ | C++ build system | JUCE 8's primary build system. Shared with cmake-js for the native addon. Single build system for all C++ code. | HIGH |
| cmake-js | 8.0.0 | Node.js native addon compilation | Bridges CMake builds into the npm ecosystem. `npm install` triggers cmake-js which builds the native addon. Out-of-the-box Electron header detection. | HIGH |
| Vite | 6.x | Frontend bundling (renderer) | Fast HMR, ESM-native, excellent React/TypeScript support. Used via electron-vite for the Electron renderer process. | HIGH |
| electron-vite | 3.x | Electron + Vite integration | Configures Vite for main/preload/renderer processes. Handles Electron-specific concerns (node integration, context isolation). | MEDIUM |
| electron-builder | 25.x | App packaging | DMG, NSIS, AppImage output. Handles code signing, auto-update, native addon bundling. | HIGH |
| pnpm | 10.x | Package manager | Faster than npm, strict dependency resolution prevents phantom deps, disk-efficient. Critical for a project with native addons where dependency correctness matters. | MEDIUM |

#### Build Pipeline

```
pnpm install
  |-> cmake-js builds native addon (C++ -> .node file)
  |-> Vite dev server starts (renderer process)
  |-> electron-vite starts Electron (main process)

pnpm run build
  |-> cmake-js release build
  |-> Vite production build
  |-> electron-builder packages everything
```

---

### Development Tools

| Technology | Purpose | Why |
|------------|---------|-----|
| Vitest | Unit/integration testing (TypeScript) | Vite-native, fast, compatible with React Testing Library |
| Catch2 | Unit testing (C++) | Modern C++ test framework, header-only, excellent for JUCE projects |
| Playwright | E2E testing (Electron) | Official Electron support for end-to-end testing |
| ESLint + Prettier | Code quality (TypeScript) | Standard tooling |
| clang-format | Code quality (C++) | Standard C++ formatting |
| clang-tidy | Static analysis (C++) | Catches bugs before runtime |

---

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Audio Framework | JUCE 8 | RTAudio + PortMIDI | No DSP, no synth primitives, no format readers. You'd rebuild 80% of JUCE. |
| Audio Framework | JUCE 8 | FAUST | DSP language, not a framework. Could complement JUCE for DSP authoring but can't replace it. |
| Native Bridge | node-addon-api | Neon (Rust) | Adds Rust to a C++ project. Wrong language. |
| Native Bridge | node-addon-api | FFI (node-ffi-napi) | Higher overhead per call, less type-safe, can't wrap complex C++ classes cleanly. |
| Build Tool | cmake-js | node-gyp | GYP is dead upstream. cmake-js aligns with JUCE's CMake and has better Electron support. |
| State Management | Zustand | Redux Toolkit | 10x more boilerplate for the same result. Zustand's selector subscriptions are better for DAW state (many independent tracks). |
| State Management | Zustand | Jotai | Atomic model is awkward for DAW state where you need cross-atom transactions (e.g., move notes across tracks). Zustand's single-store model is simpler. |
| AI Integration | AI SDK (Vercel) | LangChain.js | Over-engineered for direct API calls with tools. Adds 50+ transitive deps. AI SDK is lighter and better typed. |
| Canvas Rendering | PixiJS 8 | Konva.js | Konva is canvas-only (no WebGL). PixiJS's WebGL rendering is 10-50x faster for scenes with thousands of elements (MIDI notes). |
| Canvas Rendering | PixiJS 8 | Three.js | 3D engine for a 2D interface. Massive overkill. |
| App Shell | Electron | Tauri | No Node.js runtime = no native addon bridge to C++. Wrong architecture. |
| Waveform Display | wavesurfer.js | peaks.js (BBC) | wavesurfer.js has broader adoption, better plugin ecosystem, and more active maintenance. |

---

## Installation

```bash
# Initialize project
pnpm init

# Core Electron + React
pnpm add electron react react-dom
pnpm add -D electron-vite vite @vitejs/plugin-react typescript

# State Management
pnpm add zustand

# Canvas Rendering
pnpm add pixi.js @pixi/react wavesurfer.js

# Styling
pnpm add -D tailwindcss @tailwindcss/vite

# AI Integration
pnpm add ai @ai-sdk/anthropic @ai-sdk/openai zod

# Native Addon Bridge
pnpm add node-addon-api
pnpm add -D cmake-js

# Packaging
pnpm add -D electron-builder

# Testing
pnpm add -D vitest @testing-library/react playwright
```

```bash
# System-level dependencies (macOS)
brew install cmake

# System-level dependencies (Ubuntu/Debian)
sudo apt install cmake build-essential libasound2-dev libfreetype-dev

# JUCE (clone into vendor/)
git clone --branch 8.0.12 https://github.com/juce-framework/JUCE.git vendor/JUCE
```

---

## Version Compatibility Matrix

| Component | Version | Node.js | Electron | Notes |
|-----------|---------|---------|----------|-------|
| node-addon-api | 8.5.0 | 18+ | 35+ | N-API version 9 |
| cmake-js | 8.0.0 | 14.15+ | 35+ | Auto-detects Electron headers |
| AI SDK | 6.x | 18+ | N/A (main process) | Runs in main process only |
| PixiJS | 8.x | N/A | N/A (renderer) | WebGL 2 required (all modern Electron versions) |
| JUCE | 8.0.12 | N/A | N/A | C++17 minimum, CMake 3.24+ |
| React | 19.x | N/A | N/A (renderer) | Required for @pixi/react 8.x |

---

## Sources

- [JUCE Releases (GitHub)](https://github.com/juce-framework/JUCE/releases) -- JUCE 8.0.12 version confirmation
- [JUCE Roadmap Q1 2025](https://juce.com/blog/juce-roadmap-update-q1-2025/) -- MIDI 2.0 and text rendering plans
- [Node-API Documentation](https://nodejs.org/api/n-api.html) -- Official Node.js N-API reference
- [node-addon-api (GitHub)](https://github.com/nodejs/node-addon-api) -- C++ wrapper for N-API
- [cmake-js (GitHub)](https://github.com/cmake-js/cmake-js) -- CMake-based native addon builds
- [Electron Native Code Guide](https://www.electronjs.org/docs/latest/tutorial/native-code-and-electron) -- Native addon integration
- [Electron Native Modules](https://www.electronjs.org/docs/latest/tutorial/using-native-node-modules) -- Building native modules for Electron
- [AI SDK Documentation](https://ai-sdk.dev/docs/introduction) -- Vercel AI SDK
- [AI SDK 6 Announcement](https://vercel.com/blog/ai-sdk-6) -- Agent abstraction, latest features
- [@anthropic-ai/sdk (npm)](https://www.npmjs.com/package/@anthropic-ai/sdk) -- Anthropic TypeScript SDK
- [PixiJS v8 Launch](https://pixijs.com/blog/pixi-v8-launches) -- WebGL/WebGPU rendering improvements
- [wavesurfer.js](https://wavesurfer.xyz/) -- Waveform visualization library
- [Zustand (GitHub)](https://github.com/pmndrs/zustand) -- State management
- [electron-vite](https://electron-vite.org/) -- Electron + Vite build tooling
- [Electron Forge Vite Plugin](https://www.electronforge.io/config/plugins/vite) -- Experimental status noted
- [Electron IPC Best Practices](https://www.electronjs.org/docs/latest/tutorial/ipc) -- Async IPC patterns
- [Electron Performance Guide](https://www.electronjs.org/docs/latest/tutorial/performance) -- SharedArrayBuffer, avoid sync IPC
- [libsndfile (GitHub)](https://github.com/libsndfile/libsndfile) -- Audio file I/O
- [libsamplerate (GitHub)](https://github.com/libsndfile/libsamplerate) -- Sample rate conversion
