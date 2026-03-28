# Phase 3: Command Dispatcher & State - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — discuss skipped)

<domain>
## Phase Boundary

All DAW operations flow through a single command dispatcher with full undo/redo and JSON-serializable project state. This phase builds the command pattern infrastructure on top of Phase 2's audio engine.

Delivers: Command dispatcher that routes all operations (parameter changes, track edits, transport actions), undo/redo with 100+ operation history stack, JSON-serializable project state readable by external consumers (future AI layer), event emission for all state changes, and parameter addressability by ID.

Does NOT deliver: instruments, effects, UI, or AI integration — those are later phases. This phase creates the command infrastructure that all future operations flow through.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1-2 patterns:
- Engine is a singleton (Engine::getInstance()) — dispatcher should integrate with this
- Lock-free communication via LockFreeQueue is established — use for command dispatch to audio thread
- Transport already has atomic state — transport commands should wrap existing Transport methods
- Native bridge uses ThreadSafeFunction + Promise pattern — extend for command dispatch APIs
- IPC uses 'engine:*' namespace — extend for 'command:*' or similar

Requirements mapped to this phase: ARCH-01, ARCH-02, ARCH-05, ARCH-06, PROJ-03

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `calliope::Engine` singleton (engine/include/calliope/engine.h) — owns AudioGraph, Transport
- `calliope::Transport` (engine/include/calliope/transport.h) — atomic state, BPM, time sig, loop
- `calliope::LockFreeQueue<T>` (engine/include/calliope/lock_free_queue.h) — SPSC queue for audio thread
- `calliope::AudioGraph` (engine/include/calliope/audio_graph.h) — AudioProcessorGraph wrapper
- Native bridge pattern (native/src/bridge.cpp) — ThreadSafeFunction + Promise
- IPC handlers (app/src/main/index.ts) — 'engine:*' namespace

### Established Patterns
- Command pattern for engine operations (Phase 2 uses direct method calls — Phase 3 wraps these)
- JSON serialization via juce_core JSON classes
- Event emission pattern TBD — this phase establishes it

### Integration Points
- engine/ — new command dispatcher classes
- native/src/bridge.cpp — command dispatch and state query bridge functions
- app/src/main/index.ts — IPC handlers for commands
- app/src/preload/index.ts — expose command APIs to renderer

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure phase. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — infrastructure phase.

</deferred>

---

*Phase: 03-command-dispatcher-state*
*Context gathered: 2026-03-28*
