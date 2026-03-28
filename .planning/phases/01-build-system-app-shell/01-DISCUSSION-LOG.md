# Phase 1: Build System & App Shell - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-28
**Phase:** 01-build-system-app-shell
**Areas discussed:** Project structure, Build & dev workflow, Native addon boundary, Electron process split

---

## Project Structure

| Option | Description | Selected |
|--------|-------------|----------|
| Unified monorepo | Single repo, top-level folders: engine/, app/, native/, vendor/. One git history, coordinated versioning. | ✓ |
| Nested engine | Engine lives inside Electron project as subfolder. Tighter coupling, harder to test independently. | |
| pnpm workspaces | Monorepo with pnpm workspaces: packages/engine, packages/app. More structure upfront. | |

**User's choice:** Unified monorepo
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Git submodule | vendor/JUCE as git submodule pinned to v8.0.12 | ✓ |
| CMake FetchContent | CMake downloads JUCE at configure time | |
| System install | Require JUCE pre-installed | |
| You decide | Claude's discretion | |

**User's choice:** Git submodule
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Single CMake tree | Root CMakeLists.txt includes engine/ as static lib, native/ links against it. One cmake-js invocation. | ✓ |
| Separate builds | Engine builds independently, native addon links against built library. | |
| You decide | Claude's discretion | |

**User's choice:** Single CMake tree
**Notes:** User also specified: cross-platform from day one (macOS, Linux, Windows) — develops on all three.

---

## Build & Dev Workflow

| Option | Description | Selected |
|--------|-------------|----------|
| Manual rebuild | Run 'pnpm build:native' for C++ changes. Electron+React has HMR. | |
| Watch + auto-rebuild | File watcher triggers cmake-js rebuild automatically. | |
| You decide | Claude's discretion | ✓ |

**User's choice:** You decide
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| pnpm | Strict deps, faster, disk-efficient. Recommended in tech stack. | |
| npm | Zero setup, ships with Node.js. | |
| You decide | Claude's discretion | ✓ |

**User's choice:** You decide
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Local only for now | Phase 1 local builds only. CI later. | ✓ |
| Include basic CI | GitHub Actions on all 3 platforms. | |
| You decide | Claude's discretion | |

**User's choice:** Local only for now
**Notes:** None

---

## Native Addon Boundary

| Option | Description | Selected |
|--------|-------------|----------|
| Version + audio info | C++ returns JUCE version and audio devices. Proves bridge + JUCE init. | ✓ |
| Simple ping/pong | C++ receives/returns a string. Minimal proof. | |
| You decide | Claude's discretion | |

**User's choice:** Version + audio info
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Sync for Phase 1 | Simple synchronous Napi calls. Async comes in Phase 2. | |
| Async from the start | Use Napi::ThreadSafeFunction even for Phase 1. | ✓ |
| You decide | Claude's discretion | |

**User's choice:** Async from the start
**Notes:** None

---

## Electron Process Split

| Option | Description | Selected |
|--------|-------------|----------|
| Main process | Addon loads in main process. Renderer talks via IPC. | |
| Utility process | utilityProcess.fork() runs addon in child process. | |
| You decide | Claude's discretion | ✓ |

**User's choice:** You decide
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal proof-of-life | Single page showing JUCE version and audio devices from bridge. | |
| Skeleton layout | Basic DAW layout shell plus bridge proof-of-life. | |
| You decide | Claude's discretion | |

**User's choice:** Other — test tone button (sine wave) for proof of life
**Notes:** User wants enough UI to test actual sound output. Clarified that a simple sine wave test tone is sufficient — "proof of life and functionality for future sound add-ons." Full instruments/songs are Phase 4 scope.

---

## Claude's Discretion

- Native C++ rebuild strategy (manual vs auto-rebuild)
- Package manager (pnpm vs npm)
- Where native addon loads (main process vs utility process)

## Deferred Ideas

None — discussion stayed within phase scope
