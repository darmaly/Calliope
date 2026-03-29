# Phase 8: Mixer - Pending Discuss Decisions

**Status:** Grey area proposals generated, awaiting user acceptance
**Resume:** Run `/gsd:autonomous --from 8` to continue

## Proposed Grey Area Answers (all recommended, user has not yet accepted/rejected)

### Area 1/3: Mixer Layout & Visual Design

| # | Question | Recommended |
|---|----------|-------------|
| 1 | Where does mixer appear? | Separate tab/panel, toggled from toolbar button (like Piano Roll toggle) |
| 2 | Channel strip layout? | Vertical strips left-to-right, scrollable horizontally |
| 3 | Fader rendering? | DOM-based vertical range slider with custom styling |
| 4 | Level meter rendering? | Canvas 2D animated bars (green/yellow/red gradient) |

### Area 2/3: Controls & Interactions

| # | Question | Recommended |
|---|----------|-------------|
| 1 | Pan knob style? | Rotary knob (CSS rotation, pointer drag), -100L to +100R |
| 2 | Effect insert UI? | Dropdown slot list per strip (add/remove/reorder/bypass) |
| 3 | Volume range? | -inf to +6dB with dB markings |
| 4 | Master strip? | Rightmost strip, visually wider, always visible, own insert chain |

### Area 3/3: Engine Integration & State

| # | Question | Recommended |
|---|----------|-------------|
| 1 | Level meter data source? | Poll C++ engine via IPC at ~30Hz |
| 2 | State management? | Extend existing timeline-store (mute/solo/volume/pan already on Track) |
| 3 | Effect parameter editing? | Click slot to open parameter popover with sliders |

## Resume Instructions

When resuming, the autonomous workflow should:
1. Present these proposals to the user for acceptance
2. On acceptance, write 08-CONTEXT.md
3. Continue with UI-SPEC → plan → execute flow
