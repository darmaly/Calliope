---
phase: 2
slug: audio-engine-core
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework (TS)** | Vitest 3.2.4 |
| **Framework (C++)** | Catch2 v3 (needs setup via FetchContent) |
| **Config file (TS)** | vitest.config.ts (via package.json scripts) |
| **Config file (C++)** | None — Wave 0 installs |
| **Quick run command (TS)** | `pnpm test` |
| **Quick run command (C++)** | `cmake --build build --target calliope_engine_tests && ./build/engine/tests/calliope_engine_tests` |
| **Full suite command** | `pnpm test && cmake --build build --target calliope_engine_tests && ./build/engine/tests/calliope_engine_tests` |
| **Estimated runtime** | ~15 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pnpm test` (TS) + C++ unit test binary
- **After every plan wave:** Run full suite (both TS and C++)
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 15 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 0 | - | infra | `cmake --build build --target calliope_engine_tests` | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 1 | ENG-01 | integration (C++) | C++ test: verify AudioBuffer<float> format and sample rate | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 1 | ENG-02 | integration (C++) | C++ test: init with various buffer sizes, verify callback numSamples | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 1 | ENG-04 | unit (C++) | C++ test: add processor to master chain, verify signal | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 1 | ENG-05 | unit (C++) | C++ test: Transport state transitions, PositionInfo correctness | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 1 | ENG-06 | unit (C++) | C++ test: MetronomeProcessor generates at beat boundaries | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 1 | ARCH-04 | unit (C++) | C++ test: push/pop LockFreeQueue SPSC correctness | ❌ W0 | ⬜ pending |
| 02-XX-XX | XX | 2 | Bridge | integration (TS) | `pnpm test:native` | Partial | ⬜ pending |
| 02-XX-XX | XX | 2 | IPC | integration (TS) | `pnpm test:app` | Partial | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `engine/tests/CMakeLists.txt` — Catch2 test target setup via FetchContent
- [ ] `engine/tests/test_transport.cpp` — covers ENG-05
- [ ] `engine/tests/test_lock_free_queue.cpp` — covers ARCH-04
- [ ] `engine/tests/test_metronome.cpp` — covers ENG-06
- [ ] `engine/tests/test_audio_graph.cpp` — covers ENG-01, ENG-02, ENG-04
- [ ] `test/native.test.ts` — extend existing todo stubs for new bridge functions
- [ ] `test/app.test.ts` — extend existing todo stubs for new IPC handlers

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Audible metronome click at correct tempo | ENG-06 | Requires human ear to confirm click sound and timing perception | Launch app, set BPM to 120, enable metronome, listen for 8 beats |
| No glitches during buffer size change | ENG-02 | Audible artifacts detectable only by ear | Play audio, change buffer from 512 to 1024, listen for clicks/pops |
| Transport feel (responsive play/stop) | ENG-05 | Latency perception requires human evaluation | Press play/stop rapidly, confirm no perceivable delay |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 15s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
