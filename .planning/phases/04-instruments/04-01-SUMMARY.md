---
phase: 04-instruments
plan: 01
subsystem: audio-engine
tags: [juce, synthesizer, polyblep, ladder-filter, adsr, lfo, midi, dsp]

# Dependency graph
requires:
  - phase: 02-audio-engine
    provides: AudioProcessor pattern, AudioProcessorGraph, juce_dsp module
  - phase: 03-command-state
    provides: CommandDispatcher, ParameterRegistry, atomic parameter pattern
provides:
  - PolySynthProcessor with 16-voice polyphony, dual PolyBLEP oscillators, LadderFilter, ADSR envelopes, LFO
  - BassSynthProcessor with 4-voice polyphony, sub-oscillator, LadderFilter LPF24
  - Thread-safe MIDI injection pattern (SpinLock + MidiBuffer) for instrument processors
affects: [04-instruments, 06-timeline, 08-ai-integration]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Instrument as AudioProcessor wrapping juce::Synthesiser"
    - "PolyBLEP bandlimited waveform generation for saw/square"
    - "Per-voice LadderFilter with ADSR envelope modulation"
    - "Thread-safe MIDI injection via SpinLock + pending MidiBuffer"
    - "Atomic parameter reading from parent processor in voice renderNextBlock"

key-files:
  created:
    - engine/include/calliope/instruments/poly_synth.h
    - engine/include/calliope/instruments/poly_synth_voice.h
    - engine/include/calliope/instruments/poly_synth_sound.h
    - engine/include/calliope/instruments/bass_synth.h
    - engine/include/calliope/instruments/bass_synth_voice.h
    - engine/include/calliope/instruments/bass_synth_sound.h
    - engine/src/instruments/poly_synth.cpp
    - engine/src/instruments/poly_synth_voice.cpp
    - engine/src/instruments/bass_synth.cpp
    - engine/src/instruments/bass_synth_voice.cpp
    - engine/tests/test_poly_synth.cpp
    - engine/tests/test_bass_synth.cpp
  modified:
    - engine/CMakeLists.txt
    - engine/tests/CMakeLists.txt

key-decisions:
  - "PolyBLEP over wavetable for oscillator anti-aliasing (simpler, sufficient for 16 voices)"
  - "LadderFilter process() via AudioBlock wrapper since processSample is protected in JUCE 8"
  - "Sine sub-oscillator for BassSynth (clean low-end, CPU efficient)"

patterns-established:
  - "Instrument AudioProcessor: wraps juce::Synthesiser, uses BusesProperties stereo output, addMidiEvent for thread-safe injection"
  - "Voice DSP chain: oscillator -> filter (with envelope) -> amp envelope -> stereo output via addSample"

requirements-completed: [INST-01, INST-02]

# Metrics
duration: 7min
completed: 2026-03-28
---

# Phase 4 Plan 1: PolySynth and BassSynth Summary

**PolyBLEP-based PolySynth (16-voice, dual osc, LadderFilter, LFO) and BassSynth (4-voice, sub-oscillator) with full Catch2 test coverage**

## Performance

- **Duration:** 7 min (399s)
- **Started:** 2026-03-28T09:02:57Z
- **Completed:** 2026-03-28T09:09:36Z
- **Tasks:** 2
- **Files modified:** 14

## Accomplishments
- PolySynthProcessor with 16-voice polyphony, dual bandlimited oscillators (saw/square/sine/triangle), LadderFilter LPF24 with envelope modulation, amp ADSR, and LFO targeting pitch/filter/amp
- BassSynthProcessor with 4-voice low polyphony, main oscillator + sine sub-oscillator (1-2 octaves below), LadderFilter LPF24 optimized for bass frequency range
- Thread-safe MIDI injection pattern reusable across all instrument processors
- 12 Catch2 tests total (7 PolySynth + 5 BassSynth) all passing, full 102-test suite green with zero regressions

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement PolySynthProcessor with 16-voice polyphony** - `21e17a5` (feat)
2. **Task 2: Implement BassSynthProcessor with sub-oscillator** - `e909908` (feat)

## Files Created/Modified
- `engine/include/calliope/instruments/poly_synth.h` - PolySynthProcessor AudioProcessor with 20 atomic parameters
- `engine/include/calliope/instruments/poly_synth_voice.h` - PolySynthVoice with dual osc, filter, envelopes, LFO
- `engine/include/calliope/instruments/poly_synth_sound.h` - SynthesiserSound accepting all notes/channels
- `engine/src/instruments/poly_synth.cpp` - Processor implementation with MIDI merging and master gain
- `engine/src/instruments/poly_synth_voice.cpp` - Full DSP chain: PolyBLEP osc, LadderFilter, ADSR, LFO
- `engine/include/calliope/instruments/bass_synth.h` - BassSynthProcessor with 15 atomic parameters
- `engine/include/calliope/instruments/bass_synth_voice.h` - BassSynthVoice with main+sub oscillator
- `engine/include/calliope/instruments/bass_synth_sound.h` - SynthesiserSound accepting all notes/channels
- `engine/src/instruments/bass_synth.cpp` - Processor implementation with 4-voice Synthesiser
- `engine/src/instruments/bass_synth_voice.cpp` - DSP chain: PolyBLEP main osc, sine sub-osc, LadderFilter
- `engine/tests/test_poly_synth.cpp` - 7 Catch2 tests covering audio output, envelopes, waveforms, filter, LFO, polyphony
- `engine/tests/test_bass_synth.cpp` - 5 Catch2 tests covering audio output, sub-osc, voice stealing, filter, silence
- `engine/CMakeLists.txt` - Added poly_synth.cpp, poly_synth_voice.cpp, bass_synth.cpp, bass_synth_voice.cpp
- `engine/tests/CMakeLists.txt` - Added test_poly_synth.cpp, test_bass_synth.cpp

## Decisions Made
- Used PolyBLEP over wavetable for bandlimited oscillators: simpler implementation, sufficient CPU performance for 16 voices, matches research recommendation
- LadderFilter processSample is protected in JUCE 8, so used process() with single-sample AudioBlock wrapper instead
- BassSynth sub-oscillator always uses sine wave for clean low-end regardless of main oscillator waveform selection

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] LadderFilter processSample is protected in JUCE 8**
- **Found during:** Task 1 (PolySynthProcessor implementation)
- **Issue:** Plan specified `filter_.processSample(sample, 0)` but this method is protected in JUCE 8's LadderFilter
- **Fix:** Wrapped single sample in AudioBlock and used `filter_.process(context)` instead
- **Files modified:** engine/src/instruments/poly_synth_voice.cpp, engine/src/instruments/bass_synth_voice.cpp
- **Verification:** Build succeeds, all filter tests pass
- **Committed in:** 21e17a5 (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Minimal - API compatibility fix, same filter behavior.

## Issues Encountered
- Initial polyphony test failed due to phase cancellation between close notes through filter. Fixed by using sine waves at octave-separated frequencies for cleaner comparison.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Both synth processors ready for AudioGraph integration and ParameterRegistry registration (Plan 04-03)
- Thread-safe MIDI injection pattern established for bridge command integration
- All existing tests continue to pass (102/102)

## Self-Check: PASSED

- All 12 created files verified present on disk
- Commit 21e17a5 (Task 1) verified in git log
- Commit e909908 (Task 2) verified in git log
- Full test suite: 102/102 tests passing

---
*Phase: 04-instruments*
*Completed: 2026-03-28*
