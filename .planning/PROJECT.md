# LuneyTunes

## What This Is

An AI-powered digital audio workstation that lets anyone create professional-grade music through natural language prompts. Users describe what they want — a sound, a melody, a full arrangement — and AI agents operate the DAW directly, acting as a virtual producer with access to every tool a human would have.

## Core Value

A person with a musical vision but no DAW experience can go from an empty project to a finished, professional-sounding track by describing what they want to an AI agent.

## Requirements

### Validated

- [x] Electron-based application shell with cross-platform support (macOS, Windows, Linux) — Validated in Phase 1: Build System & App Shell
- [x] Native bridge (node-addon-api) connects C++ audio engine to Electron/Node.js layer — Validated in Phase 1: Build System & App Shell
- [x] Hybrid audio engine (C++/JUCE) with real-time multi-track audio processing, transport controls, and lock-free threading — Validated in Phase 2: Audio Engine Core
- [x] Single command dispatcher for all DAW operations with undo/redo and JSON-serializable state — Validated in Phase 3: Command Dispatcher & State

- [x] Built-in synthesizers (polysynth, bass synth) and sample-based drum machine — Validated in Phase 4: Instruments

- [x] Audio effects processing (EQ, compressor, reverb, delay, limiter) with per-track insert chains — Validated in Phase 5: Effects Processing

- [x] Multi-track timeline with clip creation, editing, and grid snapping — Validated in Phase 6: Timeline & Arrangement

- [x] Piano roll MIDI editor with note drawing, selection, velocity control, and quantization — Validated in Phase 7: Piano Roll

- [x] Mixer with per-track channel strips (volume, pan, mute/solo, effect inserts, level meters) and master strip — Validated in Phase 8: Mixer

### Active
- [ ] AI agent integration layer (BYOK — user provides their own API key for Claude, GPT, etc.)
- [ ] AI has full DAW access — sound design, composition, arrangement, mixing, mastering
- [ ] Natural language interface for all DAW operations (chat-based interaction)
- [x] Piano roll editor with note drawing, velocity, and quantization — Validated in Phase 7: Piano Roll
- [ ] Timeline/arrangement view with track management
- [x] Mixer with per-track controls (volume, pan, sends) — Validated in Phase 8: Mixer
- [ ] Built-in synthesizers and sample-based instruments
- [ ] Built-in effects (EQ, compression, reverb, delay, stereo imaging, limiter)
- [ ] Sound/preset selection and browsing system
- [ ] AI-assisted mixing and mastering chain
- [x] Export to WAV, MP3, FLAC, and individual stems — Validated in Phase 9: Project Management & Export
- [x] Project save/load — Validated in Phase 9: Project Management & Export

- [x] Unified workspace with transport bar, keyboard shortcuts, and panel focus management — Validated in Phase 10: Application Integration

### Out of Scope

- VST/AU plugin hosting — complex to implement, defer to post-v1; built-in instruments cover v1 needs
- Cloud/collaborative features — single-user local app for v1
- Mobile version — desktop-first
- Built-in AI subscription service — BYOK model keeps costs on the user side
- Video/film scoring features — music production only for v1
- Hardware controller support — mouse/keyboard + AI for v1

## Context

The target user is someone with musical taste and ideas but who finds traditional DAWs overwhelming. They know what good music sounds like, they can describe what they want ("old school west coast sine wave synth lead", "melodic drum and bass"), but they can't navigate the technical complexity of a professional DAW. The AI bridges that gap.

The AI agent model is "bring your own key" — users connect their preferred AI provider (Anthropic Claude, OpenAI GPT, etc.) via API key. The AI receives a structured representation of the current DAW state and can issue commands to any DAW subsystem: load instruments, write MIDI, adjust mixer settings, apply effects, trigger renders.

Architecture is a hybrid stack:
- **C++ (JUCE)** — real-time audio engine: synthesis, DSP, effects, MIDI processing, audio I/O
- **Electron** — application shell: UI (React), project management, file handling
- **Node.js** — AI integration layer: API key management, prompt routing, DAW command translation

The C++ engine exposes an API that the Electron/Node layer calls into via native addons (node-addon-api/napi). The AI layer translates natural language into structured DAW commands.

## Constraints

- **Audio Engine**: C++ with JUCE framework — real-time audio requirements demand native performance
- **App Shell**: Electron — cross-platform, large ecosystem, proven for complex UIs, easy AI SDK integration
- **AI Model**: BYOK (bring your own key) — no server infrastructure needed for AI, users pay providers directly
- **Platform**: Desktop (macOS, Windows, Linux) — Electron handles cross-platform
- **Audio Quality**: Professional-grade — 44.1kHz/48kHz sample rates, 32-bit float internal processing minimum

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| C++ (JUCE) for audio engine | Industry standard for DAWs, mature DSP/MIDI/plugin framework, maximum audio performance | — Pending |
| Electron for app shell | Cross-platform, Node.js AI SDK integration, large UI ecosystem (React), proven for complex apps | — Pending |
| BYOK for AI access | No server costs, user flexibility in AI provider choice, simpler infrastructure | — Pending |
| Built-in instruments only for v1 | VST/AU hosting is complex; built-in synths give AI full programmatic control | — Pending |
| Hybrid architecture (C++ engine + JS/TS layer) | Balances audio performance with AI integration ease — similar to Bitwig's approach | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd:transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd:complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-03-28 after Phase 6 completion*
