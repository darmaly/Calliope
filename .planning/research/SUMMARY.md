# Project Research Summary

**Project:** LuneyTunes (AI-Powered DAW)
**Domain:** AI-native digital audio workstation (hybrid C++/Electron desktop application)
**Researched:** 2026-03-27
**Confidence:** HIGH

## Executive Summary

LuneyTunes is a desktop DAW where an AI agent is the primary interface -- the user describes music in natural language, and the AI operates a real audio engine (synthesizers, effects, mixer, timeline) programmatically. This is not a chatbot bolted onto an existing DAW, nor a black-box generator like Suno. The recommended architecture is a three-layer system: a C++/JUCE real-time audio engine at the bottom, a Node.js coordination layer (project state, command dispatch, AI integration) in the middle, and a React/PixiJS UI in an Electron renderer at the top. The native bridge between C++ and Node.js uses node-addon-api (N-API) built with cmake-js, which is the only production-viable approach for wrapping JUCE in Electron.

The build order is strictly bottom-up: audio engine first, then native bridge, then Electron shell with command dispatcher, then UI and AI integration in parallel. This is non-negotiable because every layer depends on the one below it. The AI layer and UI are both consumers of the same command dispatcher, meaning they can be developed concurrently once the dispatcher exists. Built-in instruments and effects thread through all phases -- basic versions early, refined versions later.

The top risks are: (1) blocking the real-time audio thread with allocations or locks, causing audible glitches that destroy credibility; (2) the IPC bridge between C++ and Electron becoming a serialization bottleneck at scale; (3) three-way state synchronization (C++ engine, Node.js state, React UI) drifting out of consistency; and (4) AI hallucinating invalid DAW commands. All four must be addressed architecturally from day one -- none can be retrofitted. The mitigation strategy is a strict unidirectional data flow with a single command dispatcher, lock-free audio thread communication, SharedArrayBuffer for high-frequency data, and a deterministic validation layer between the AI and the command API.

## Key Findings

### Recommended Stack

The stack is anchored by JUCE 8 for the C++ audio engine (no viable alternative at this maturity level for DSP, synthesis, MIDI, and cross-platform audio I/O) and Electron 35 for the desktop shell (required for Node.js native addon integration). The bridge uses node-addon-api with cmake-js, aligning with JUCE's CMake build system. The frontend uses React 19, Zustand for state, PixiJS 8 for GPU-accelerated canvas rendering (piano roll, waveforms, meters), and Tailwind for non-canvas UI chrome. AI integration uses Vercel's AI SDK 6 for multi-provider BYOK support with Zod schema validation.

**Core technologies:**
- **JUCE 8 (C++):** Audio engine framework -- industry standard for DSP, synthesis, MIDI, audio I/O; no alternative offers comparable maturity
- **node-addon-api + cmake-js:** Native bridge -- ABI-stable across Electron versions, shares CMake build system with JUCE
- **Electron 35:** Desktop shell -- cross-platform with Node.js integration for native addons; required architecture
- **React 19 + PixiJS 8:** UI layer -- React for controls/chat, PixiJS for GPU-rendered piano roll/waveforms/meters
- **Zustand 4.5:** State management -- selector-based subscriptions prevent re-render cascades across many tracks
- **AI SDK 6 (Vercel):** AI integration -- unified multi-provider API with streaming, tool calling, and structured output; BYOK-native
- **Zod 3:** Schema validation -- defines AI tool call schemas and validates DAW commands from AI responses

### Expected Features

**Must have (table stakes):**
- Multi-track timeline with clip management and arrangement view
- Piano roll / MIDI editor with quantize and snap
- Mixer with per-track volume, pan, mute, solo, and effect insert chains
- Transport controls (play, stop, record, loop, BPM, time signature)
- At least 2 built-in synthesizers + 1 sampler (no VST in v1, so built-in instruments are mandatory)
- Core effects: parametric EQ, compressor, reverb, delay, limiter
- Audio export to WAV, MP3, FLAC
- Project save/load with autosave and undo/redo
- Natural language chat interface with BYOK API key management
- AI generates MIDI, selects instruments/presets, applies effects, adjusts mixer, explains actions
- Full DAW state visibility for the AI (structured project representation in every prompt)

**Should have (differentiators):**
- End-to-end "describe a song, get a produced track" workflow (the killer feature)
- AI-driven song structure and arrangement (intro/verse/chorus/bridge)
- Genre-aware production templates
- Multiple AI "takes" / alternatives for user selection
- Context-aware AI suggestions ("Your verse needs a bass line")
- Visual feedback of AI actions (notes appearing, faders moving in real-time)
- Guided onboarding via AI conversation

**Defer (v2+):**
- VST/AU plugin hosting (enormous complexity; AI cannot control arbitrary plugins)
- Audio recording from microphone (hardware dependency, latency management)
- Stem separation, audio-to-MIDI transcription (enhancement, not foundation)
- AI mastering chain (basic limiter on master sufficient for MVP)
- Natural language sound design (preset selection sufficient for MVP)
- Collaboration, mobile, notation view, video sync

### Architecture Approach

Three-process architecture with strict unidirectional data flow. The C++ audio engine runs real-time DSP on a lock-free audio thread, communicating with a message thread via FIFO queues. The Node.js main process owns the authoritative project state, runs the command dispatcher (single entry point for all operations from UI and AI), and hosts the AI integration layer. The React renderer subscribes to state patches via Electron IPC and renders canvas (PixiJS) and DOM components. Every mutation -- whether from a mouse click, keyboard shortcut, or AI tool call -- flows through the same command dispatcher, guaranteeing undoability and consistency.

**Major components:**
1. **C++ Audio Engine (JUCE)** -- Real-time DSP processing, synthesis, effects, audio I/O; two-thread model (audio thread + message thread) with lock-free communication
2. **Native Addon Bridge (node-addon-api)** -- Wraps C++ engine as Node.js module; ObjectWrap for lifetime, ThreadSafeFunction for callbacks, SharedArrayBuffer for high-frequency data
3. **Command Dispatcher (Node.js main process)** -- Single chokepoint for all DAW operations; validates, executes against immutable project state, pushes diffs to engine and UI, manages undo/redo stack
4. **AI Integration Layer (Node.js main process)** -- BYOK key management, prompt construction with project state, AI tool definitions mapped 1:1 to dispatcher commands, response parsing
5. **React/PixiJS Renderer** -- Canvas-rendered piano roll, waveforms, meters, timeline; DOM-rendered transport, track headers, chat, settings; Zustand store mirrors main process state

### Critical Pitfalls

1. **Blocking the audio thread (CP-1)** -- Any allocation, lock, or syscall in processBlock() causes audible glitches. Prevent with lock-free FIFOs, atomics, pre-allocated buffers, and real-time safety assertions from day one. This is the single most common mistake in audio software.
2. **IPC bridge bottleneck (CP-2)** -- Serializing meter/waveform/position data across the native boundary at 30-60fps causes UI stutter and memory leaks. Prevent with SharedArrayBuffer for high-frequency data, batched async IPC, and a formal versioned protocol.
3. **Three-way state desynchronization (CP-3)** -- C++ engine, Node.js state, and React UI drift apart, causing stale displays, conflicting AI commands, and broken undo. Prevent with single authoritative state in Node.js, unidirectional data flow, and command/event architecture.
4. **AI hallucinating DAW commands (CP-4)** -- AI references non-existent tracks, invalid parameters, or impossible configurations. Prevent with deterministic validation layer before execution, typed tool schemas with enums and ranges, and current state snapshots in each prompt.
5. **Web UI performance collapse (CP-5)** -- DOM + Canvas rendering overwhelms the renderer with realistic project sizes. Prevent with WebGL (PixiJS) for data-dense views, virtualization everywhere, pre-computed waveform peaks from C++, and minimal React involvement in frame-rate-sensitive areas.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Foundation -- C++ Audio Engine and Build System
**Rationale:** Everything depends on the audio engine. No sound means no product. The build pipeline (CMake + cmake-js + Electron) is the second critical dependency -- a broken build blocks all progress. Start on macOS only.
**Delivers:** Working C++ audio engine with basic DSP graph, single synth voice, transport (play/pause/seek), audio I/O via JUCE AudioDeviceManager, lock-free FIFO infrastructure, and a cmake-js build that produces a .node addon loadable in Electron.
**Addresses:** Audio engine foundation, transport controls, build tooling
**Avoids:** CP-1 (establish lock-free patterns from the start), MP-1 (single-platform first, CI pipeline early)

### Phase 2: Native Bridge and Command Dispatcher
**Rationale:** The bridge and dispatcher are the spine of the application. AI and UI both depend on this layer. Designing the command pattern here determines undo/redo, state flow, and AI parity for all future phases.
**Delivers:** node-addon-api ObjectWrap around the engine, basic method exposure (transport, parameter setting, MIDI note playback), ThreadSafeFunction for meter callbacks, project state model in TypeScript, command dispatcher with undo/redo, Electron shell with IPC to a minimal renderer.
**Addresses:** DAW command API, project save/load, undo/redo
**Avoids:** CP-2 (design IPC protocol formally), CP-3 (establish single state ownership in Node.js), MP-4 (command-level undo from day one)

### Phase 3: Built-in Instruments and Effects
**Rationale:** The AI needs instruments to work with. Without built-in synths and effects, there is nothing for the AI to control. This phase can begin early (C++ DSP work) but integration testing requires the bridge from Phase 2.
**Delivers:** At least 2 synthesizers (subtractive polysynth, wavetable/bass), 1 sampler with WAV loading, core effects (parametric EQ, compressor, reverb, delay, limiter), preset system with named presets the AI can reference.
**Addresses:** Built-in synthesizers, built-in effects, built-in sampler, preset management
**Avoids:** mP-4 (use plugin-like interfaces even for internal instruments, easing future VST support)

### Phase 4: Core UI -- Timeline, Piano Roll, Mixer
**Rationale:** Users need to see and manually interact with the DAW. The AI also benefits from visual feedback. This phase can run in parallel with Phase 5 (AI integration) since both consume the command dispatcher independently.
**Delivers:** PixiJS-rendered timeline/arrangement view with clips, piano roll MIDI editor, mixer with channel strips and real-time meters, transport bar with playhead, waveform display for audio clips, Zustand state store mirroring main process.
**Addresses:** Multi-track timeline, piano roll, mixer, waveform display, visual feedback
**Avoids:** CP-5 (WebGL from the start, virtualization, pre-computed peaks from C++)

### Phase 5: AI Integration Layer
**Rationale:** This is the core product differentiator. It depends on the command dispatcher (Phase 2) and instruments/effects (Phase 3) but not on the UI (Phase 4). Can be developed in parallel with Phase 4.
**Delivers:** BYOK API key management, AI SDK integration with Claude/GPT/Gemini providers, tool definitions mapped 1:1 to dispatcher commands, prompt construction with hierarchical project state, streaming responses, chat interface component, AI can create tracks, load instruments, write MIDI, adjust mixer, apply effects.
**Addresses:** Natural language chat, AI MIDI generation, AI instrument selection, AI effect application, AI mixer control, conversational iteration
**Avoids:** CP-4 (deterministic validation layer, typed schemas), MP-3 (hierarchical state representation), MP-5 (async design, graceful degradation, streaming)

### Phase 6: End-to-End AI Workflows and Polish
**Rationale:** With all subsystems working, build the high-level AI workflows that compose multiple operations into production workflows. This is where the "describe a song, get a produced track" experience comes together.
**Delivers:** End-to-end song creation from description, AI-driven arrangement and song structure, genre-aware templates, AI mixing workflow, multiple AI takes/alternatives, context-aware suggestions, visual feedback of AI actions, guided onboarding, export to WAV/MP3/FLAC.
**Addresses:** Differentiator features, production quality, onboarding
**Avoids:** MP-3 (optimized context management for complex projects), MP-4 (AI undo grouping with transaction semantics)

### Phase Ordering Rationale

- **Bottom-up dependency chain:** Audio engine -> bridge -> dispatcher -> (UI + AI in parallel) -> integrated workflows. This matches both the architecture's dependency graph and the feature dependency graph from FEATURES.md.
- **Phases 4 and 5 are parallel:** The architecture deliberately separates UI and AI as independent consumers of the command dispatcher. This enables two workstreams after Phase 3.
- **Instruments/effects (Phase 3) before AI (Phase 5):** The AI needs something to control. Shipping AI without instruments is like shipping a chef without a kitchen.
- **Polish and integration last (Phase 6):** End-to-end workflows require all subsystems. Premature integration wastes effort on changing interfaces.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 1 (Audio Engine):** JUCE 8 CMake integration with cmake-js requires specific configuration research; lock-free patterns for the specific command types need detailed design
- **Phase 2 (Native Bridge):** ThreadSafeFunction patterns for meter data, SharedArrayBuffer setup across Electron processes, and the exact IPC protocol design need spike work
- **Phase 3 (Instruments):** Synthesizer DSP implementation (oscillators, filters, envelopes) is deep domain knowledge; preset format design affects AI usability
- **Phase 5 (AI Integration):** Prompt engineering for DAW context, hierarchical state serialization format, and tool schema design need iterative experimentation

Phases with standard patterns (skip research-phase):
- **Phase 4 (Core UI):** PixiJS rendering, Zustand state management, and Electron IPC are well-documented with established patterns
- **Phase 6 (End-to-End Workflows):** Primarily composition of existing subsystems; standard integration work

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | JUCE, Electron, node-addon-api, React, AI SDK are all mature, well-documented technologies with clear version targets. No speculative choices. |
| Features | MEDIUM-HIGH | Table stakes are well-defined by competitive landscape. Differentiator feasibility (especially end-to-end AI production) depends on LLM capability which is proven but untested at this integration depth. |
| Architecture | HIGH | Three-process model with command dispatcher is validated by existing JUCE+Electron projects (ElectronMeetsJUCE), DAW architecture literature, and Electron best practices. |
| Pitfalls | HIGH | All critical pitfalls sourced from production post-mortems, JUCE forum expertise, Electron documentation, and established audio development literature. |

**Overall confidence:** HIGH

### Gaps to Address

- **Synthesizer DSP design:** Research identified JUCE's `juce_dsp` module but did not prescribe specific synth architectures (oscillator types, filter topology, modulation routing). Needs domain-specific design during Phase 3 planning.
- **AI prompt engineering for DAW context:** The hierarchical state representation concept is sound but the actual prompt format, context budget allocation, and tool schema details require iterative experimentation during Phase 5.
- **Cross-platform build validation:** Research recommends starting on macOS. Windows and Linux builds with JUCE + cmake-js + Electron have known friction points that need CI validation before Phase 2 completion.
- **Audio format licensing:** LAME (MP3 encoding) is LGPL; bundling strategy (static vs. dynamic linking) has licensing implications that need legal review.
- **Electron 35 + node-addon-api 8 compatibility:** Version matrix looks correct but needs build verification in the initial scaffolding spike.
- **PixiJS + React 19 integration maturity:** @pixi/react 8.x is relatively new; may encounter edge cases in complex DAW UI scenarios.

## Sources

### Primary (HIGH confidence)
- [JUCE 8 Documentation and GitHub Releases](https://github.com/juce-framework/JUCE/releases) -- Audio engine framework, DSP modules, build system
- [Node-API Official Documentation](https://nodejs.org/api/n-api.html) -- Native addon ABI stability, threading model
- [Electron IPC and Performance Guides](https://www.electronjs.org/docs/latest/tutorial/ipc) -- IPC patterns, SharedArrayBuffer, native module integration
- [Timur Doumler: Using Locks in Real-Time Audio Processing](https://timur.audio/using-locks-in-real-time-audio-processing-safely) -- Lock-free audio thread patterns
- [AI SDK 6 Documentation](https://ai-sdk.dev/docs/introduction) -- Multi-provider AI integration, tool calling, streaming
- [Billy DM: DAW Frontend Development Struggles](https://billydm.github.io/blog/daw-frontend-development-struggles/) -- Web rendering performance for DAW UIs

### Secondary (MEDIUM confidence)
- [ElectronMeetsJUCE (GitHub)](https://github.com/Serge45/ElectronMeetsJUCE) -- Proof of concept for JUCE + Electron integration
- [How to Build a Modern DAW (Misko Lee)](https://medium.com/@Misko_Lee/how-to-build-a-modern-digital-audio-workstation-daw-d1f4a2c670e1) -- DAW architecture overview
- [Building a Simple AI DAW (Jon Aylor)](https://jonaylor.com/blog/building-a-simple-ai-daw-part-2-mcp-and-agents) -- AI agent integration patterns for DAWs
- [Suno Studio, MIDI Agent, Producer.ai](https://suno.com/blog/suno-studio) -- Competitive landscape for AI music tools
- [Electron IPC Memory Leak Issue #27039](https://github.com/electron/electron/issues/27039) -- Known IPC pitfall

### Tertiary (LOW confidence)
- AI context window overflow impact on DAW state -- Logical extrapolation; actual impact depends on model context sizes at deployment time
- End-to-end AI production workflow feasibility -- No direct precedent at this integration depth; high confidence in components, medium confidence in composition

---
*Research completed: 2026-03-27*
*Ready for roadmap: yes*
