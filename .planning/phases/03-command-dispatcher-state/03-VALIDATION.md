---
phase: 3
slug: command-dispatcher-state
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework (C++)** | Catch2 v3 (already configured) |
| **Framework (TS)** | Vitest (already configured) |
| **Config file (C++)** | engine/tests/CMakeLists.txt |
| **Config file (TS)** | vitest.config.ts |
| **Quick run command (C++)** | `cd build && ctest --test-dir engine/tests -R "dispatcher" --output-on-failure` |
| **Quick run command (TS)** | `pnpm test` |
| **Full suite command** | `cd build && ctest --output-on-failure && cd .. && pnpm test` |
| **Estimated runtime** | ~20 seconds |

---

## Sampling Rate

- **After every task commit:** `cd build && ctest -R "dispatcher\|project_state\|parameter\|undo" --output-on-failure`
- **After every plan wave:** Full C++ test suite + pnpm test
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 20 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 03-XX-XX | XX | 0 | - | infra | `cmake --build build --target calliope_engine_tests` | ❌ W0 | ⬜ pending |
| 03-XX-XX | XX | 1 | ARCH-01 | unit (C++) | `ctest -R "dispatcher"` | ❌ W0 | ⬜ pending |
| 03-XX-XX | XX | 1 | ARCH-06 | unit (C++) | `ctest -R "event_emission"` | ❌ W0 | ⬜ pending |
| 03-XX-XX | XX | 1 | PROJ-03 | unit (C++) | `ctest -R "undo_redo"` | ❌ W0 | ⬜ pending |
| 03-XX-XX | XX | 1 | ARCH-02 | unit (C++) | `ctest -R "project_state"` | ❌ W0 | ⬜ pending |
| 03-XX-XX | XX | 1 | ARCH-05 | unit (C++) | `ctest -R "parameter_registry"` | ❌ W0 | ⬜ pending |
| 03-XX-XX | XX | 2 | Bridge | integration (TS) | `pnpm test` | Partial | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `engine/tests/test_command_dispatcher.cpp` — covers ARCH-01, ARCH-06, PROJ-03
- [ ] `engine/tests/test_project_state.cpp` — covers ARCH-02
- [ ] `engine/tests/test_parameter_registry.cpp` — covers ARCH-05
- [ ] Add `juce::juce_data_structures` to engine/CMakeLists.txt target_link_libraries
- [ ] Add new test files to engine/tests/CMakeLists.txt

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None | - | - | All phase behaviors have automated verification |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 20s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
