---
phase: 1
slug: build-system-app-shell
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | vitest (TypeScript) / Catch2 (C++) |
| **Config file** | none — Wave 0 installs |
| **Quick run command** | `pnpm test` |
| **Full suite command** | `pnpm test:all` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pnpm test`
- **After every plan wave:** Run `pnpm test:all`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01 | 1 | ARCH-03 | integration | `pnpm test:native` | ❌ W0 | ⬜ pending |
| 01-02-01 | 02 | 1 | UI-01 | e2e | `pnpm test:app` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `vitest` — install and configure for TypeScript tests
- [ ] `test/native.test.ts` — stub for native addon loading verification
- [ ] `test/app.test.ts` — stub for Electron app launch verification

*If none: "Existing infrastructure covers all phase requirements."*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Audio output (sine wave test tone) | ARCH-03 | Requires speaker/headphone to verify audible output | Launch app, click test tone button, verify sine wave plays through speakers |
| Cross-platform build | UI-01 | Requires each OS to verify | Run `pnpm build:native && pnpm dev` on macOS, Linux, Windows |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
