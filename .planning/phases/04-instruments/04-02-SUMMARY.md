---
phase: 04-instruments
plan: 02
subsystem: audio-engine
tags: [juce, sampler, drum-machine, midi, wav, audio-formats, synthesiser]

# Dependency graph
requires:
  - phase: 01-build-system
    provides: "JUCE CMake build system, engine static library target"
  - phase: 02-audio-engine
    provides: "AudioProcessor pattern (MetronomeProcessor), audio graph infrastructure"
provides:
  - "DrumMachineProcessor AudioProcessor for sample-based drum playback"
  - "16-pad GM drum map (MIDI notes 36-51) with WAV/AIFF/MP3 loading"
  - "Thread-safe MIDI injection pattern for sample instruments"
affects: [05-effects, 06-ui, 10-integration]

# Tech tracking
tech-stack:
  added: [juce::SamplerSound, juce::SamplerVoice, juce::AudioFormatManager]
  patterns: [SamplerSound-per-pad mapping, SpinLock MIDI injection, Synthesiser voice pooling]

key-files:
  created:
    - engine/include/calliope/instruments/drum_machine.h
    - engine/src/instruments/drum_machine.cpp
    - engine/tests/test_drum_machine.cpp
  modified:
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt

key-decisions:
  - "16 SamplerVoice instances for simultaneous pad playback (one per pad)"
  - "JUCE SamplerSound/SamplerVoice over hand-rolled sample interpolation"
  - "GM drum standard mapping starting at MIDI note 36 (C1)"

patterns-established:
  - "Sample instrument pattern: AudioFormatManager + Synthesiser + SamplerSound/SamplerVoice"
  - "Pad-to-MIDI mapping: padIndex + kFirstMidiNote for GM compatibility"

requirements-completed: [INST-03]

# Metrics
duration: 165s
completed: 2026-03-28
---

# Phase 04 Plan 02: DrumMachine/Sampler Summary

**DrumMachineProcessor with 16-pad GM drum map using JUCE SamplerSound/SamplerVoice for WAV/AIFF/MP3 sample playback triggered via MIDI**

## Performance

- **Duration:** 2m 45s
- **Started:** 2026-03-28T09:03:08Z
- **Completed:** 2026-03-28T09:05:53Z
- **Tasks:** 1
- **Files modified:** 5

## Accomplishments
- DrumMachineProcessor AudioProcessor with 16 pads mapped to MIDI notes 36-51 (GM drum standard)
- Sample loading from disk via AudioFormatManager (WAV, AIFF, MP3 support via registerBasicFormats)
- Thread-safe MIDI injection using SpinLock + MidiBuffer pattern matching existing engine conventions
- 10 Catch2 tests covering playback, silence, GM mapping, volume control, sample management, and edge cases
- Full test suite (90 tests) passes with zero regressions

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement DrumMachineProcessor with sample loading and MIDI-triggered playback** - `015c014` (feat)

## Files Created/Modified
- `engine/include/calliope/instruments/drum_machine.h` - DrumMachineProcessor class with 16-pad sample instrument API
- `engine/src/instruments/drum_machine.cpp` - Full implementation: sample loading, MIDI routing, Synthesiser rendering
- `engine/tests/test_drum_machine.cpp` - 10 Catch2 tests with synthetic WAV generation for sample playback verification
- `engine/CMakeLists.txt` - Added drum_machine.cpp to engine library sources
- `engine/tests/CMakeLists.txt` - Added test_drum_machine.cpp to test executable

## Decisions Made
- Used 16 SamplerVoice instances (one per pad) to allow all pads to play simultaneously
- Used JUCE SamplerSound/SamplerVoice built-in classes instead of hand-rolled sample interpolation -- cubic interpolation and pitch shifting come for free
- GM drum standard starting at MIDI note 36 (C1) for industry compatibility
- Attack time 0.001s, release time 0.01s, max sample length 30s for responsive drum triggering

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- DrumMachineProcessor ready for integration into audio graph alongside synth instruments
- Sample loading API ready for bridge exposure to Electron/Node.js layer
- GM drum map compatible with standard MIDI controllers and AI-generated drum patterns

---
*Phase: 04-instruments*
*Completed: 2026-03-28*
