---
phase: 10
slug: application-integration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-29
---

# Phase 10 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest 3.x |
| **Config file** | `vitest.config.ts` (project root) |
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
| 10-01-01 | 01 | 1 | UI-04 | unit | `npx vitest run test/time-format.test.ts` | ❌ W0 | ⬜ pending |
| 10-01-02 | 01 | 1 | UI-02 | tsc | `npx tsc --noEmit` | N/A | ⬜ pending |
| 10-02-01 | 02 | 2 | UI-05 | unit | `npx vitest run test/keyboard-shortcuts.test.ts` | ❌ W0 | ⬜ pending |
| 10-02-02 | 02 | 2 | UI-03 | unit | `npx vitest run test/waveform-utils.test.ts` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `test/time-format.test.ts` — covers UI-04 (beat-to-time, beat-to-bars formatting)
- [ ] `test/keyboard-shortcuts.test.ts` — covers UI-05 (shortcut routing by panel focus)

*Existing `test/waveform-utils.test.ts` covers UI-03.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Transport bar visible and functional | UI-04 | Visual layout verification | Open app, verify top bar with play/stop/BPM/position |
| Panel focus border appears on click | UI-02 | Visual indicator | Click panels, verify accent border appears |
| Waveform clips render in timeline | UI-03 | WebGL rendering requires app | Add audio clips, verify waveforms visible |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
