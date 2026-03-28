# Phase 1: Build System & App Shell - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver a working hybrid application where Electron successfully loads and calls into a C++ native addon built with JUCE and cmake-js. Includes a test tone (sine wave) to prove audio output works end-to-end. No audio engine, instruments, or effects — just the build system, bridge, and proof-of-life.

</domain>

<decisions>
## Implementation Decisions

### Project Structure
- **D-01:** Unified monorepo layout — top-level folders: `engine/` (C++/JUCE), `native/` (Node addon bridge), `app/` (Electron/React), `vendor/` (JUCE submodule)
- **D-02:** JUCE included as git submodule at `vendor/JUCE`, pinned to v8.0.12
- **D-03:** Single CMake tree — root `CMakeLists.txt` includes `engine/` as a static lib target, `native/` links against it. One cmake-js invocation builds everything
- **D-04:** Cross-platform from day one — build system must work on macOS, Linux, and Windows (developer works on all three)

### Build & Dev Workflow
- **D-05:** Local builds only for Phase 1 — no CI setup (deferred to later)
- **D-06:** Electron + React dev server uses electron-vite with HMR for the renderer

### Native Addon Boundary
- **D-07:** First round-trip returns JUCE version string and available audio devices — proves bridge works AND JUCE is properly initialized
- **D-08:** Async from the start — use `Napi::ThreadSafeFunction` pattern even for Phase 1 queries, establishing the async pattern for Phase 2's real-time audio needs

### Electron Process Architecture
- **D-09:** UI includes a test tone button (simple sine wave via JUCE audio output) as proof-of-life for the full audio path: React → IPC → main process → C++ addon → JUCE → speakers

### Claude's Discretion
- Native C++ rebuild strategy (manual command vs file watcher with auto-rebuild)
- Package manager choice (pnpm vs npm)
- Where the native addon loads (main process vs utility process)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Tech Stack
- `CLAUDE.md` §Technology Stack — Full stack decisions, version pins, and alternatives considered
- `CLAUDE.md` §Installation — Dependency installation commands for all platforms

### Requirements
- `.planning/REQUIREMENTS.md` §Architecture — ARCH-03 (native bridge), referenced by this phase
- `.planning/REQUIREMENTS.md` §User Interface — UI-01 (Electron + React shell), referenced by this phase

### Research Flags
- `.planning/STATE.md` §Blockers/Concerns — "JUCE 8 CMake integration with cmake-js needs configuration spike"

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- None — green field project, no existing code

### Established Patterns
- None yet — Phase 1 establishes the foundational patterns

### Integration Points
- Root `CMakeLists.txt` will be the entry point for the entire C++ build
- `package.json` at root will orchestrate JS-side builds and native addon compilation via cmake-js
- electron-vite config will manage main/preload/renderer process builds

</code_context>

<specifics>
## Specific Ideas

- Test tone button should output a simple sine wave — "proof of life and functionality for future sound add-ons"
- Developer works on macOS, Linux, and Windows — cross-platform is a hard requirement, not a nice-to-have

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-build-system-app-shell*
*Context gathered: 2026-03-28*
