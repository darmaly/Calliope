---
phase: 4
slug: instruments
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 v3 (already configured) |
| **Config file** | engine/tests/CMakeLists.txt |
| **Quick run command** | `cd build && ctest --test-dir engine/tests -R "PolySynth\|BassSynth\|DrumMachine\|Instrument" --output-on-failure` |
| **Full suite command** | `cd build && ctest --test-dir engine/tests --output-on-failure` |
| **Estimated runtime** | ~20 seconds |

---

## Sampling Rate

- **After every task commit:** `cmake --build build --target calliope_engine_tests && cd build && ctest --test-dir engine/tests --output-on-failure`
- **After every plan wave:** Full suite
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 20 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 04-XX-XX | XX | 1 | INST-01 | unit (C++) | `ctest -R "PolySynth"` | ❌ W0 | ⬜ pending |
| 04-XX-XX | XX | 1 | INST-02 | unit (C++) | `ctest -R "BassSynth"` | ❌ W0 | ⬜ pending |
| 04-XX-XX | XX | 2 | INST-03 | unit (C++) | `ctest -R "DrumMachine"` | ❌ W0 | ⬜ pending |
| 04-XX-XX | XX | 2 | INST-04 | unit (C++) | `ctest -R "Instrument"` | ❌ W0 | ⬜ pending |
| 04-XX-XX | XX | 3 | Bridge | integration (TS) | `pnpm test` | Partial | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `engine/tests/test_poly_synth.cpp` — covers INST-01
- [ ] `engine/tests/test_bass_synth.cpp` — covers INST-02
- [ ] `engine/tests/test_drum_machine.cpp` — covers INST-03
- [ ] `engine/tests/test_instrument_commands.cpp` — covers INST-04
- [ ] Add test files to `engine/tests/CMakeLists.txt`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Polysynth sounds musical | INST-01 | Subjective audio quality | Play notes via console, listen for clean sine/saw/square |
| Bass synth low-end presence | INST-02 | Subjective tonal quality | Trigger low notes, confirm sub-bass presence |
| Drum samples sound correct | INST-03 | Requires loaded samples | Load WAV, trigger pads, listen for sample playback |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 20s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
