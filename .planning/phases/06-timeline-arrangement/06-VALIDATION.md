---
phase: 6
slug: timeline-arrangement
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 6 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest 3.2.4 |
| **Config file** | vitest.config.ts (root) |
| **Quick run command** | `pnpm test` |
| **Full suite command** | `pnpm test:all` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** `pnpm vitest run test/timeline-store.test.ts test/beat-math.test.ts`
- **After every plan wave:** `pnpm test:all`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 06-XX | XX | 1 | TL-01 | unit + integration | `pnpm vitest run test/timeline-store.test.ts` | ❌ W0 | ⬜ pending |
| 06-XX | XX | 1 | TL-04 | unit | `pnpm vitest run test/timeline-store.test.ts` | ❌ W0 | ⬜ pending |
| 06-XX | XX | 1 | TL-05 | unit | `pnpm vitest run test/beat-math.test.ts` | ❌ W0 | ⬜ pending |
| 06-XX | XX | 2 | TL-02 | unit | `pnpm vitest run test/clip-operations.test.ts` | ❌ W0 | ⬜ pending |
| 06-XX | XX | 2 | TL-03 | unit | `pnpm vitest run test/waveform-utils.test.ts` | ❌ W0 | ⬜ pending |
| 06-XX | XX | 2 | TL-06 | unit | `pnpm vitest run test/timeline-store.test.ts` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `test/timeline-store.test.ts` — covers TL-01, TL-04, TL-06
- [ ] `test/beat-math.test.ts` — covers TL-05
- [ ] `test/clip-operations.test.ts` — covers TL-02
- [ ] `test/waveform-utils.test.ts` — covers TL-03

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Timeline visual rendering | TL-01 | Canvas rendering requires visual inspection | Launch app, verify tracks render, scroll/zoom, playhead moves |
| Clip drag interactions | TL-02 | Mouse interaction testing | Create clip by click+drag, move, resize, verify snap |
| Waveform display | TL-03 | Visual fidelity check | Place audio clip, verify waveform renders correctly |
| Track color coding | TL-04 | Color appearance | Add tracks, assign colors, verify visual distinction |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
