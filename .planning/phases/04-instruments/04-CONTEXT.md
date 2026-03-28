# Phase 4: Instruments - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — discuss skipped)

<domain>
## Phase Boundary

Users can load built-in synthesizers and a sampler that produce sound through the audio engine. This phase builds instrument processors on top of Phase 2's audio graph and Phase 3's command dispatcher.

Delivers: Polysynth with configurable oscillators/filter/envelopes/LFO, bass synth with sub-oscillator, drum machine/sampler that loads WAV/MP3 samples and maps to pads/keys. All instrument parameters controllable via the command dispatcher.

Does NOT deliver: effects, UI, timeline, or piano roll — those are later phases.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1-3 patterns:
- Instruments are AudioProcessor subclasses that plug into the AudioProcessorGraph
- Each instrument registers its parameters with ParameterRegistry for ID-based control
- Instrument commands go through CommandDispatcher (undoable parameter changes)
- JUCE juce_dsp module provides oscillators, filters, envelopes (already linked)
- JUCE juce_audio_formats provides WAV/MP3 reading for sampler
- Bridge pattern extends with instrument-specific commands

Requirements mapped to this phase: INST-01, INST-02, INST-03, INST-04

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `calliope::AudioGraph` — AudioProcessorGraph for routing instruments to master bus
- `calliope::CommandDispatcher` — dispatch instrument parameter commands
- `calliope::ParameterRegistry` — register instrument parameters by string ID
- `calliope::ProjectState` — serialize instrument state to JSON
- `calliope::MasterBusProcessor` — instruments route through master bus
- juce_dsp: dsp::Oscillator, dsp::LadderFilter, dsp::Compressor
- juce_audio_formats: AudioFormatManager for WAV/MP3/AIFF reading

### Established Patterns
- AudioProcessor subclass for each processor (see MetronomeProcessor, MasterBusProcessor)
- Atomic parameters for real-time-safe control
- Commands accept component references for testability
- Bridge functions use ThreadSafeFunction + Promise

### Integration Points
- engine/CMakeLists.txt — add new instrument source files
- AudioGraph — add instrument nodes, connect to master bus
- Engine — manage instrument lifecycle, register parameters
- native/src/bridge.cpp — add instrument bridge functions
- app/src/main/index.ts — add instrument IPC handlers

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

*Phase: 04-instruments*
*Context gathered: 2026-03-28*
