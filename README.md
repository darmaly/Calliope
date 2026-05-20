# Calliope

An AI-powered digital audio workstation that lets anyone create professional-grade music through natural language. Describe what you want -- a sound, a melody, a full arrangement -- and AI agents operate the DAW directly, acting as a virtual producer.

## Status

Early development (v0.1.0). Core audio engine and UI foundations are functional.

## Architecture

- **Audio Engine** -- C++ with [JUCE 8](https://juce.com/) for real-time audio processing, synthesis, and effects
- **Native Bridge** -- node-addon-api + cmake-js connecting C++ engine to Node.js
- **App Shell** -- Electron 35 with electron-vite
- **Frontend** -- React 19, TypeScript, Zustand, PixiJS 8, Tailwind CSS 4
- **AI** -- BYOK (bring your own key) via Vercel AI SDK -- no server needed

## What's Built

- Real-time audio engine with multi-track mixing, transport, and parameter automation
- Synthesizers (poly synth, bass synth) and drum machine with sample loading
- Effects chain (reverb, delay, compressor, EQ, chorus) with lock-free processing
- Full native bridge: commands, events, undo/redo, state serialization
- Timeline UI with PixiJS canvas, clip management, drag/drop, zoom/scroll
- Piano roll with MIDI note editing, velocity lane, quantize, keyboard shortcuts

## Prerequisites

- Node.js 22+ and pnpm 10+
- CMake 3.24+
- C++17 compiler (Xcode CLT on macOS, MSVC on Windows, GCC/Clang on Linux)
- JUCE dependencies (see [JUCE docs](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) for Linux)

## Setup

```bash
git clone https://github.com/darmaly/Calliope.git
cd Calliope
git submodule update --init --recursive
pnpm install
```

## Development

```bash
# Build native engine + app and launch
pnpm start

# Dev mode with hot reload (UI only)
pnpm dev

# Run tests
pnpm test
```

## Project Structure

```
engine/          C++ audio engine (JUCE)
native/          Node.js native addon bridge
app/             Electron app (main + renderer)
  src/main/      Electron main process
  src/preload/   Context bridge
  src/renderer/  React UI
test/            Vitest test suites
vendor/          JUCE submodule
```

## License

All rights reserved.
