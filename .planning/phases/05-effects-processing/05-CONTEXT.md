# Phase 5: Effects Processing - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — discuss skipped)

<domain>
## Phase Boundary

Built-in audio effects can be applied to tracks via insert chains with per-track routing. This phase builds effect processors and the insert chain infrastructure on top of Phase 2's audio graph and Phase 3's command dispatcher.

Delivers: Parametric EQ (4-band), compressor, algorithmic reverb, tempo-synced delay, and brick-wall limiter. Per-track insert chain with insert/remove/reorder/bypass. Master bus limiter for final output loudness control. All effect parameters controllable via the command dispatcher.

Does NOT deliver: UI, timeline, piano roll, or mixer views — those are Phases 6-10.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1-4 patterns:
- Effects are AudioProcessor subclasses that plug into insert chains
- MasterBusProcessor already has insert chain concept — extend for per-track chains
- Each effect registers parameters with ParameterRegistry for ID-based control
- Effect commands go through CommandDispatcher (undoable parameter changes, insert/remove/reorder)
- JUCE juce_dsp provides dsp::Compressor, dsp::Reverb, dsp::LadderFilter, dsp::IIR
- Bridge pattern extends with effect-specific commands

Requirements mapped to this phase: FX-01, FX-02, FX-03, FX-04, FX-05, FX-06, ENG-03

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `calliope::MasterBusProcessor` — already has atomic volume, extend with insert chain
- `calliope::CommandDispatcher` — dispatch effect parameter commands
- `calliope::ParameterRegistry` — register effect parameters by string ID
- `calliope::ProjectState` — serialize effect state to JSON
- `calliope::AudioGraph` — routing effects in processor graph
- juce_dsp: dsp::Compressor, dsp::Reverb, dsp::IIR::Filter, dsp::ProcessorChain

### Established Patterns
- AudioProcessor subclass for each processor (PolySynth, BassSynth, DrumMachine, Metronome, MasterBus)
- Atomic parameters for real-time-safe control
- Commands accept component references for testability
- Bridge functions use ThreadSafeFunction + Promise

### Integration Points
- engine/CMakeLists.txt — add new effect source files
- AudioGraph/MasterBus — integrate insert chains
- Engine — manage effect lifecycle, register parameters
- native/src/bridge.cpp — add effect bridge functions
- app/src/main/index.ts — add effect IPC handlers

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

*Phase: 05-effects-processing*
*Context gathered: 2026-03-28*
