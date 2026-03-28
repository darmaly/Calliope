---
phase: 01-build-system-app-shell
plan: 02
subsystem: electron-app-shell
tags: [electron, react, ipc, native-bridge, electron-vite]
dependency_graph:
  requires: [01-01]
  provides: [electron-shell, react-ui, ipc-bridge, test-tone-ui]
  affects: [02-01, 02-02]
tech_stack:
  added: [electron-vite, "@vitejs/plugin-react"]
  patterns: [contextBridge-ipc, electron-vite-config, native-addon-loading]
key_files:
  created:
    - app/electron.vite.config.ts
    - app/src/main/index.ts
    - app/src/main/native-bridge.ts
    - app/src/preload/index.ts
    - app/src/renderer/index.html
    - app/src/renderer/main.tsx
    - app/src/renderer/App.tsx
    - app/src/renderer/App.css
    - app/src/renderer/components/TestTone.tsx
    - app/src/renderer/types/calliope.d.ts
    - app/tsconfig.json
    - app/tsconfig.node.json
    - electron-builder.yml
  modified:
    - package.json
decisions:
  - Used electron-vite for dev server with externalizeDepsPlugin to handle native .node files
  - contextBridge IPC pattern with strict contextIsolation and no nodeIntegration for security
  - Native addon loaded via createRequire with isPackaged path switching for dev vs production
metrics:
  duration: 77s
  completed: "2026-03-27"
---

# Phase 01 Plan 02: Electron App Shell with React UI and IPC Bridge Summary

Electron application shell wired with React 19 renderer, contextBridge IPC for secure main-renderer communication, and TestTone component exercising the full audio path from React button click through IPC to the C++/JUCE native addon.

## What Was Built

### Task 1: Electron app with electron-vite, IPC bridge, and React renderer

Created the complete Electron application structure using electron-vite:

- **electron-vite config** (`app/electron.vite.config.ts`): Three-target Vite config for main/preload/renderer with `externalizeDepsPlugin` and `.node` file externalization for native addon compatibility.
- **Main process** (`app/src/main/index.ts`): Creates BrowserWindow with `contextIsolation: true`, `nodeIntegration: false`, `sandbox: false`. Registers three IPC handlers (`engine:getInfo`, `engine:startTestTone`, `engine:stopTestTone`) that delegate to the native addon.
- **Native bridge** (`app/src/main/native-bridge.ts`): Typed wrapper around the native addon using `createRequire` with `app.isPackaged` path switching for dev vs production builds.
- **Preload script** (`app/src/preload/index.ts`): Exposes `window.calliope` API via `contextBridge.exposeInMainWorld` with three methods mapping to IPC invoke calls.
- **React renderer**: Entry point (`main.tsx`), root component (`App.tsx`) fetching engine info on mount, `TestTone` component with play/stop toggle and frequency slider, dark theme CSS, and TypeScript declarations for the `window.calliope` API.
- **electron-builder.yml**: Packaging config with `asarUnpack` for `.node` files, targeting DMG (macOS), NSIS (Windows), AppImage (Linux).
- **package.json**: Added `dev`, `build:app`, `build`, `start`, and `pack` scripts plus `main` entry point.

### Task 2: Verify full audio path (auto-approved)

Auto-approved in autonomous execution mode. The wiring is complete from React -> IPC -> main process -> native addon -> JUCE.

## Commits

| Task | Commit | Message |
|------|--------|---------|
| 1 | 5955b50 | feat(01-02): create Electron app shell with React UI, IPC bridge, and test tone component |

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None - all components are wired to their data sources. The TestTone component calls through to the real native addon, and App.tsx fetches real engine info on mount.

## Decisions Made

1. **electron-vite with externalizeDepsPlugin**: Keeps native `.node` files out of the Vite bundle, letting Node.js `require()` handle them at runtime.
2. **contextBridge IPC pattern**: Strict `contextIsolation: true` + `nodeIntegration: false` for security. Renderer only accesses native functionality through the typed `window.calliope` API.
3. **createRequire for addon loading**: ESM main process uses `createRequire(import.meta.url)` to load the CommonJS native addon, with `app.isPackaged` toggling between dev and production paths.

## Self-Check: PASSED

- All 13 created files verified present on disk
- Commit 5955b50 verified in git log
