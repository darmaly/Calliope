---
phase: 7
slug: piano-roll
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 7 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest 3.x |
| **Config file** | vitest.config.ts |
| **Quick run command** | `npx vitest run test/piano-roll-store.test.ts test/note-operations.test.ts test/piano-helpers.test.ts` |
| **Full suite command** | `npx vitest run` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `npx vitest run test/piano-roll-store.test.ts test/note-operations.test.ts test/piano-helpers.test.ts`
- **After every plan wave:** Run `npx vitest run`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | PR-01 | unit | `npx vitest run test/piano-roll-store.test.ts -t "note model"` | ❌ W0 | ⬜ pending |
| 07-01-02 | 01 | 1 | PR-01 | unit | `npx vitest run test/piano-helpers.test.ts` | ❌ W0 | ⬜ pending |
| 07-01-03 | 01 | 1 | PR-02 | unit | `npx vitest run test/note-operations.test.ts` | ❌ W0 | ⬜ pending |
| 07-01-04 | 01 | 1 | PR-02 | unit | `npx vitest run test/piano-roll-store.test.ts -t "selection"` | ❌ W0 | ⬜ pending |
| 07-01-05 | 01 | 1 | PR-03 | unit | `npx vitest run test/note-operations.test.ts -t "velocity"` | ❌ W0 | ⬜ pending |
| 07-01-06 | 01 | 1 | PR-04 | unit | `npx vitest run test/note-operations.test.ts -t "quantize"` | ❌ W0 | ⬜ pending |
| 07-01-07 | 01 | 1 | PR-05 | unit | `npx vitest run test/piano-roll-store.test.ts -t "scroll"` | ❌ W0 | ⬜ pending |
| 07-01-08 | 01 | 1 | PR-05 | unit | `npx vitest run test/beat-math.test.ts -t "triplet"` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `test/piano-roll-store.test.ts` — stubs for PR-01 (note model), PR-02 (selection), PR-05 (scroll/zoom)
- [ ] `test/note-operations.test.ts` — stubs for PR-02 (CRUD), PR-03 (velocity), PR-04 (quantize)
- [ ] `test/piano-helpers.test.ts` — stubs for PR-01 (pitch names, black key detection, velocity alpha)
- [ ] Extend `test/beat-math.test.ts` with triplet snap tests — PR-05

*Existing infrastructure covers Vitest framework setup.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Piano roll renders notes on PixiJS canvas | PR-01 | Visual rendering not testable via unit tests | Launch app, open MIDI clip, verify notes display on grid |
| Note drawing via pointer interaction | PR-02 | Requires pointer event simulation on PixiJS canvas | Click+drag on empty grid, verify note appears |
| Velocity lane bar chart visualization | PR-03 | Visual rendering | Toggle velocity lane, verify bars render at correct heights |
| Piano keyboard visual display | PR-05 | Visual rendering | Verify keyboard shows on left edge with correct black/white pattern |
| Split panel resize interaction | PR-05 | Requires drag interaction | Drag divider between timeline and piano roll |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
