---
phase: 8
slug: mixer
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-29
---

# Phase 8 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest (latest, via project config) |
| **Config file** | `vitest.config.ts` |
| **Quick run command** | `npx vitest run --reporter=verbose` |
| **Full suite command** | `npx vitest run --reporter=verbose` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `npx vitest run --reporter=verbose`
- **After every plan wave:** Run `npx vitest run --reporter=verbose`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 08-01-01 | 01 | 1 | MIX-01 | unit | `npx vitest run test/mixer-store.test.ts -x` | ❌ W0 | ⬜ pending |
| 08-01-02 | 01 | 1 | MIX-01 | unit | `npx vitest run test/mixer-store.test.ts -x` | ❌ W0 | ⬜ pending |
| 08-01-03 | 01 | 1 | MIX-02 | unit | `npx vitest run test/mixer-store.test.ts -x` | ❌ W0 | ⬜ pending |
| 08-01-04 | 01 | 1 | MIX-03 | unit | `npx vitest run test/mixer-store.test.ts -x` | ❌ W0 | ⬜ pending |
| 08-01-05 | 01 | 1 | MIX-04 | unit | `npx vitest run test/mixer-helpers.test.ts -x` | ❌ W0 | ⬜ pending |
| 08-01-06 | 01 | 1 | ALL | unit | `npx vitest run test/mixer-helpers.test.ts -x` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `test/mixer-store.test.ts` — stubs for MIX-01, MIX-02, MIX-03 (store state, volume/pan actions, effect chain tracking)
- [ ] `test/mixer-helpers.test.ts` — stubs for MIX-04, ALL (dB/gain conversion, fader position mapping, meter smoothing math)

*Existing vitest infrastructure covers framework setup.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Level meters animate at ~30fps | MIX-04 | Visual animation timing cannot be unit tested | Open mixer panel, play audio, verify meters move smoothly |
| Fader drag interaction feels responsive | MIX-01 | Pointer interaction UX is subjective | Drag fader up/down, verify smooth following with no jank |
| Pan knob rotation follows pointer | MIX-01 | CSS rotation interaction is visual | Drag pan knob, verify rotation matches pointer angle |
| Effect popover opens on slot click | MIX-03 | Portal positioning is layout-dependent | Click effect slot, verify popover appears near slot |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
