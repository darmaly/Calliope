---
phase: 9
slug: project-management-export
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-29
---

# Phase 9 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest 3.x (TypeScript) + Catch2 (C++) |
| **Config file** | `vitest.config.ts` (root) + `engine/tests/CMakeLists.txt` |
| **Quick run command** | `npx vitest run --reporter=verbose` |
| **Full suite command** | `npx vitest run --reporter=verbose` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** Run `npx vitest run --reporter=verbose`
- **After every plan wave:** Run `npx vitest run --reporter=verbose`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 15 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 09-01-01 | 01 | 1 | PROJ-01 | integration | `npx vitest run test/project-io.test.ts` | ❌ W0 | ⬜ pending |
| 09-01-02 | 01 | 1 | PROJ-02 | unit | `npx vitest run test/project-store.test.ts` | ❌ W0 | ⬜ pending |
| 09-02-01 | 02 | 1 | PROJ-04 | build | `npm run build 2>&1 \| tail -20` | ❌ W0 | ⬜ pending |
| 09-02-02 | 02 | 1 | PROJ-05, PROJ-06 | build | `npm run build 2>&1 \| tail -20` | ❌ W0 | ⬜ pending |
| 09-03-01 | 03 | 2 | PROJ-07 | tsc | `npx tsc --noEmit` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `test/project-io.test.ts` — stubs for PROJ-01 (save/load roundtrip)
- [ ] `test/project-store.test.ts` — stubs for PROJ-02 (dirty flag, autosave timer)

*Existing vitest infrastructure covers framework setup.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| File picker opens correctly | PROJ-01 | Electron dialog requires running app | File > Save, verify native dialog appears |
| Autosave triggers periodically | PROJ-02 | Timer behavior requires running app | Wait 2 minutes, verify .calliope.autosave file appears |
| Export progress bar updates | PROJ-04 | Visual animation | Export WAV, verify progress bar fills smoothly |
| Unsaved changes warning on close | PROJ-01 | Window close event | Make changes, click X, verify warning dialog |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 15s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
