---
phase: 5
slug: effects-processing
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 5 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework (C++)** | Catch2 v3 (already configured) |
| **Framework (TS)** | Vitest (already configured) |
| **Config file (C++)** | engine/tests/CMakeLists.txt |
| **Quick run command** | `cmake --build build --target calliope_engine_tests && cd build && ctest --test-dir engine/tests -R "EQ\|Compressor\|Reverb\|Delay\|Limiter\|InsertChain" --output-on-failure` |
| **Full suite command** | `cmake --build build --target calliope_engine_tests && cd build && ctest --test-dir engine/tests --output-on-failure` |
| **Estimated runtime** | ~25 seconds |

---

## Sampling Rate

- **After every task commit:** Quick run command targeting new tests
- **After every plan wave:** Full suite
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 25 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 05-XX | XX | 1 | FX-01 | unit (C++) | `ctest -R "ParametricEQ"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 1 | FX-02 | unit (C++) | `ctest -R "Compressor"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 1 | FX-03 | unit (C++) | `ctest -R "Reverb"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 1 | FX-04 | unit (C++) | `ctest -R "Delay"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 1 | FX-05 | unit (C++) | `ctest -R "Limiter"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 2 | ENG-03 | unit (C++) | `ctest -R "InsertChain"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 2 | FX-06 | unit (C++) | `ctest -R "EffectCommand"` | ❌ W0 | ⬜ pending |
| 05-XX | XX | 3 | Bridge | integration | `pnpm test` | Partial | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `engine/tests/test_parametric_eq.cpp` — covers FX-01
- [ ] `engine/tests/test_compressor.cpp` — covers FX-02
- [ ] `engine/tests/test_reverb.cpp` — covers FX-03
- [ ] `engine/tests/test_delay.cpp` — covers FX-04
- [ ] `engine/tests/test_limiter.cpp` — covers FX-05
- [ ] `engine/tests/test_insert_chain.cpp` — covers ENG-03
- [ ] `engine/tests/test_effect_commands.cpp` — covers FX-06
- [ ] Add test files to engine/tests/CMakeLists.txt

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Effects sound correct | FX-01-05 | Subjective audio quality | Play instrument through effect, listen for expected processing |
| Limiter prevents clipping | FX-05 | Audible output verification | Send loud signal through limiter, confirm no distortion |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 25s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
