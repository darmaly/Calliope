# Phase 8: Mixer - Research

**Researched:** 2026-03-29
**Domain:** DAW mixer UI (channel strips, faders, meters, effect inserts) + C++ engine metering bridge
**Confidence:** HIGH

## Summary

Phase 8 builds the mixer panel UI and connects it to the existing C++ audio engine. The codebase already has all the engine-side infrastructure needed: per-track InsertChainProcessors, a MasterBusProcessor with atomic volume control, effect insert/remove/reorder/bypass commands via the command dispatcher, and a parameter registry for volume/pan addressing. What is missing and must be built: (1) the mixer panel React UI with channel strips, faders, pan knobs, and effect slot controls, (2) a new mixer-store for panel state and meter data, (3) C++ level metering infrastructure (RMS/peak computation in the audio thread, a bridge endpoint to poll levels), (4) preload/IPC extensions for meter polling and volume/pan convenience methods, and (5) extending the Track type with volume/pan fields.

The UI is entirely DOM-based (Tailwind CSS) except for level meters which use Canvas 2D. This follows established Phase 6/7 patterns. No new npm dependencies are needed. The primary technical challenge is real-time level metering: computing RMS and peak levels in the audio thread without blocking, transferring them to Node.js via a lock-free mechanism, and rendering them smoothly at ~30fps in the renderer.

**Primary recommendation:** Build bottom-up: engine metering first, then bridge/IPC, then mixer-store, then UI components (channel strip, fader, pan knob, meter, effect slots), then integration into App.tsx layout.

<user_constraints>

## User Constraints (from CONTEXT.md)

### Locked Decisions
- Mixer appears as a separate tab/panel, toggled from a toolbar button (consistent with Piano Roll toggle pattern)
- Channel strips arranged vertically left-to-right, scrollable horizontally
- Faders rendered as DOM-based vertical range sliders with custom Tailwind styling
- Level meters rendered with Canvas 2D animated bars (green/yellow/red gradient)
- Pan control is a rotary knob using CSS rotation with pointer drag, displaying -100L to +100R
- Effect insert UI uses dropdown slot list per strip with add/remove/reorder/bypass controls
- Volume range is -inf to +6dB with dB markings on the fader scale
- Master strip is rightmost, visually wider, always visible, with its own insert chain
- Level meter data polled from C++ engine via IPC at ~30Hz
- State management extends existing timeline-store (Track type gains volume/pan fields; mute/solo already present)
- Effect parameter editing via click-to-open popover with parameter sliders

### Claude's Discretion
- Specific CSS styling choices for faders, knobs, and meters
- Animation easing and smoothing for meter decay
- Exact pixel dimensions of channel strips and master strip
- Scroll behavior and overflow handling for many tracks

### Deferred Ideas (OUT OF SCOPE)
None.

</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| MIX-01 | Per-track channel strip with volume fader, pan knob, mute, and solo | ChannelStrip component with Fader, PanKnob, M/S/Arm buttons; extends Track type with volume/pan; dispatches via parameter.set command |
| MIX-02 | Master channel strip with volume fader and insert effect chain | MasterStrip component reusing ChannelStrip pattern; uses existing master.setVolume command and masterChainPtr_ InsertChain |
| MIX-03 | Per-track insert effect chain with add, remove, reorder, and bypass per effect | EffectSlotList/EffectSlot components; existing effectInsert/Remove/Reorder/Bypass preload API already works |
| MIX-04 | Visual level meters on each channel strip showing real-time signal level | Canvas 2D LevelMeter component; NEW C++ metering in audio graph processBlock; NEW bridge getMeterLevels endpoint polled at ~30Hz |

</phase_requirements>

## Standard Stack

### Core (already installed)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| React | 19.x | UI framework | Already in project |
| Zustand | 5.x | State management (mixer-store, timeline-store extensions) | Already in project, selector-based subscriptions ideal for per-track updates |
| Tailwind CSS | 4.x | Fader, knob, strip, toolbar styling | Already in project via @tailwindcss/vite |
| lucide-react | ^1.7.0 | Icons (SlidersHorizontal for toggle, Circle for bypass) | Already in project |
| node-addon-api | 8.5.0 | C++ bridge for meter data endpoint | Already in project |

### Supporting

No new dependencies needed. All mixer UI is custom-built with existing stack.

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom CSS fader | HTML `<input type="range">` | Native range inputs have inconsistent cross-platform styling; custom div-based fader gives full control over appearance and dB scaling |
| Canvas 2D meters | PixiJS meters | PixiJS is overkill for simple rectangular bar rendering; Canvas 2D matches Phase 6 ruler precedent and avoids WebGL context overhead for small elements |
| Polling meter levels | SharedArrayBuffer | SharedArrayBuffer requires COOP/COEP headers in Electron, adds complexity; polling at 30Hz via IPC is sufficient for visual meters and simpler to implement |

## Architecture Patterns

### Recommended Project Structure

```
app/src/renderer/
  stores/
    mixer-store.ts           # NEW: panel visibility, meter levels, effect chains
    timeline-store.ts        # MODIFIED: Track type gains volume + pan
  types/
    timeline.ts              # MODIFIED: Track interface gains volume, pan
    mixer.ts                 # NEW: EffectSlot, MeterLevel types
    calliope.d.ts            # MODIFIED: getMeterLevels, setTrackVolume, setTrackPan
  components/
    mixer/
      MixerPanel.tsx         # Panel container with toolbar and strip scroller
      MixerToolbar.tsx       # Toggle, view controls
      ChannelStrip.tsx       # Per-track strip (fader, pan, buttons, effects, meter)
      MasterStrip.tsx        # Master channel strip (wider, always visible)
      Fader.tsx              # Vertical volume fader with dB scale
      PanKnob.tsx            # Rotary pan control
      LevelMeter.tsx         # Canvas 2D stereo meter
      EffectSlotList.tsx     # Vertical list of effect insert slots
      EffectSlot.tsx         # Single slot (dropdown add, bypass, name)
      EffectParamPopover.tsx # Portal-based parameter editor
    timeline/
      TimelineToolbar.tsx    # MODIFIED: add mixer toggle button
  hooks/
    use-meter-polling.ts     # NEW: requestAnimationFrame loop polling meter data
  App.tsx                    # MODIFIED: add mixer panel below timeline (or below piano roll)

app/src/preload/
  index.ts                   # MODIFIED: add getMeterLevels, setTrackVolume, setTrackPan

native/src/
  bridge.cpp                 # MODIFIED: add GetMeterLevels endpoint

engine/
  include/calliope/
    audio_graph.h            # MODIFIED: add getMeterLevels() method
    master_bus.h             # MODIFIED: add peak/RMS atomics
  src/
    audio_graph.cpp          # MODIFIED: compute per-track meter levels in processBlock
    master_bus.cpp           # MODIFIED: compute master meter levels
```

### Pattern 1: Meter Data Flow (C++ -> Canvas 2D)

**What:** Real-time audio level data flows from the audio thread through atomic variables, across the IPC bridge, into Zustand store, and renders via Canvas 2D.

**When to use:** Any time real-time audio visualization is needed.

**Architecture:**

```
Audio Thread (C++)          Message Thread (C++)       Node.js Main          Renderer
processBlock() ->           getMeterLevels() ->        IPC handler ->        requestAnimationFrame
  compute RMS/peak            read atomics               return JSON           poll + setState
  store in atomics            return struct              to renderer           Canvas 2D draw
```

**Example:**

```cpp
// In audio_graph.h / processBlock - per-track RMS and peak
struct MeterData {
    std::atomic<float> rmsLeft{0.0f};
    std::atomic<float> rmsRight{0.0f};
    std::atomic<float> peakLeft{0.0f};
    std::atomic<float> peakRight{0.0f};
};

// Audio thread writes with relaxed ordering (single producer)
meterData.rmsLeft.store(rms, std::memory_order_relaxed);
meterData.peakLeft.store(peak, std::memory_order_relaxed);

// Message thread reads with relaxed ordering (single consumer)
float rms = meterData.rmsLeft.load(std::memory_order_relaxed);
```

```typescript
// use-meter-polling.ts
function useMeterPolling() {
  const updateMeters = useMixerStore((s) => s.updateMeterLevels)
  const panelVisible = useMixerStore((s) => s.panelVisible)

  useEffect(() => {
    if (!panelVisible) return
    let rafId: number
    const poll = async () => {
      const levels = await window.calliope.getMeterLevels()
      updateMeters(levels)
      rafId = requestAnimationFrame(poll)
    }
    rafId = requestAnimationFrame(poll)
    return () => cancelAnimationFrame(rafId)
  }, [panelVisible, updateMeters])
}
```

### Pattern 2: Volume dB Mapping

**What:** Linear fader position (0.0-1.0) maps to gain (0.0 to ~2.0) with logarithmic dB display.

**When to use:** Volume fader control.

**Example:**

```typescript
// dB-to-linear and linear-to-dB conversion
// Volume range: -inf to +6dB
// Internal gain: 0.0 (silence) to ~2.0 (+6dB)
function gainToDb(gain: number): number {
  if (gain <= 0) return -Infinity
  return 20 * Math.log10(gain)
}

function dbToGain(db: number): number {
  if (db <= -100) return 0  // treat as -inf
  return Math.pow(10, db / 20)
}

// Fader position (0-1) to dB using a curve that gives more resolution near 0dB
// Common approach: power curve where position^4 maps to gain
function faderToGain(position: number): number {
  // position 0 = silence, position ~0.75 = 0dB (unity), position 1.0 = +6dB
  if (position <= 0) return 0
  const maxGain = dbToGain(6)  // ~2.0
  return maxGain * Math.pow(position, 4)
}

function gainToFader(gain: number): number {
  if (gain <= 0) return 0
  const maxGain = dbToGain(6)
  return Math.pow(gain / maxGain, 0.25)
}
```

### Pattern 3: Panel Integration (Same as Piano Roll)

**What:** Mixer panel renders below the timeline (or below piano roll if both open), with a SplitDivider for resize.

**When to use:** Adding bottom panels to the main layout.

**Example (App.tsx modification):**

```typescript
// App.tsx - add mixer panel alongside piano roll
const showMixer = useMixerStore((s) => s.panelVisible)
const mixerHeight = useMixerStore((s) => s.panelHeight)

return (
  <div className="h-screen flex flex-col">
    <div className="flex-1 min-h-[200px]">
      <TimelineView />
    </div>
    {showPianoRoll && (
      <>
        <SplitDivider onDrag={handlePRDrag} />
        <div style={{ height: panelHeight }}><PianoRollPanel /></div>
      </>
    )}
    {showMixer && (
      <>
        <SplitDivider onDrag={handleMixerDrag} />
        <div style={{ height: mixerHeight }}><MixerPanel /></div>
      </>
    )}
  </div>
)
```

### Anti-Patterns to Avoid

- **Sync IPC for meter data:** Never use `ipcRenderer.sendSync` for meter polling. Always use async `ipcRenderer.invoke` or a streaming mechanism.
- **Meter polling when panel is hidden:** Always gate the requestAnimationFrame loop on `panelVisible`. Polling meters when the mixer is closed wastes CPU and IPC bandwidth.
- **Re-rendering all strips when one meter updates:** Use Zustand selectors to subscribe each LevelMeter to only its own track's meter data. Do NOT subscribe to the entire `meterLevels` record.
- **Mouse events for fader/knob:** Use pointer events (onPointerDown, onPointerMove, onPointerUp) per Phase 6/7 convention. Mouse events miss touch/pen input.
- **Blocking audio thread for meter reads:** Never use mutex or lock in the audio thread. Use `std::atomic<float>` with `memory_order_relaxed` for meter values.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Effect insert/remove/reorder/bypass | Custom IPC endpoints | Existing `window.calliope.effectInsert/Remove/Reorder/Bypass` | Already implemented in Phase 5 with full undo support |
| Parameter set (volume, pan, effect params) | Custom volume/pan endpoints | `window.calliope.dispatchCommand({ command: 'parameter.set', params: { id, value } })` | ParameterRegistry already handles all named parameters |
| Master volume | Custom master endpoint | `window.calliope.dispatchCommand({ command: 'master.setVolume', params: { volume } })` | Already implemented in Phase 2/3 with undo |
| Mute/Solo/Arm toggle | Custom mixer buttons | Reuse `toggleMute`/`toggleSolo`/`toggleArm` from timeline-store | Already bound to existing TrackHeader buttons |
| Portal-based popover | Custom overlay system | Reuse ContextMenu/portal pattern from Phase 6 | Document click dismiss, z-ordering already solved |

**Key insight:** The C++ engine already has all the audio processing infrastructure (InsertChain, MasterBus, ParameterRegistry, CommandDispatcher). This phase is primarily a UI build with one significant engine addition: level metering.

## Common Pitfalls

### Pitfall 1: Audio Thread Lock Contention from Metering

**What goes wrong:** Using mutex or allocation in the audio thread to store meter values causes audio glitches.
**Why it happens:** The audio callback runs under strict real-time constraints (e.g., 512 samples at 44100Hz = 11.6ms deadline).
**How to avoid:** Use `std::atomic<float>` for each meter value. Write with `memory_order_relaxed` in processBlock. Read with `memory_order_relaxed` in the bridge thread.
**Warning signs:** Audio dropouts that correlate with meter panel being open.

### Pitfall 2: Fader Precision at Low Volumes

**What goes wrong:** Linear fader mapping makes it impossible to make fine adjustments near 0dB where most mixing happens.
**Why it happens:** A linear 0-to-2.0 range means 0dB (1.0) is at the midpoint, giving equal resolution to the rarely-used +6dB range.
**How to avoid:** Use a power curve (position^4) so that ~75% of the fader travel covers the -inf to 0dB range. This gives the user fine control in the most-used zone.
**Warning signs:** Users complaining that small fader movements cause dramatic volume changes.

### Pitfall 3: Meter Smoothing Mismatch

**What goes wrong:** Meters look jittery or sluggish because smoothing is applied inconsistently between C++ and JS.
**Why it happens:** If both the C++ RMS computation and the JS rendering apply separate decay, the behavior doubles up. Or if neither applies it, meters jump erratically.
**How to avoid:** Compute raw RMS/peak in C++ per buffer. Apply ballistic smoothing (attack/release envelope) in JavaScript where you control the frame rate. The C++ side should report the instantaneous level per buffer.
**Warning signs:** Meters that never reach peak or that stick at high values too long.

### Pitfall 4: Canvas 2D Memory Leak from Uncleared Animation Frames

**What goes wrong:** requestAnimationFrame loops continue after component unmount, causing memory leaks and stale state updates.
**Why it happens:** Missing cleanup in useEffect return function.
**How to avoid:** Always store the rafId and call cancelAnimationFrame in the useEffect cleanup. Gate the loop on panelVisible.
**Warning signs:** Increasing memory usage over time, "setState on unmounted component" warnings.

### Pitfall 5: IPC Flooding from Per-Track Meter Polling

**What goes wrong:** Calling getMeterLevels once per track per frame creates N IPC calls at 30fps (e.g., 16 tracks = 480 IPC calls/sec).
**Why it happens:** Naive per-track polling.
**How to avoid:** Single getMeterLevels call returns ALL track levels in one response object. One IPC round-trip per frame.
**Warning signs:** High CPU usage in main process, visible frame drops in the mixer.

### Pitfall 6: Track ID Mismatch Between UI and Engine

**What goes wrong:** The UI creates tracks with `crypto.randomUUID()` IDs, but the C++ engine has hardcoded instrument names ("polysynth", "basssynth", "drumMachine") for insert chains.
**Why it happens:** Phase 5 insert chains are keyed by instrument name, not by arbitrary track IDs.
**How to avoid:** The mixer must map UI track IDs to engine track IDs. Currently the engine uses instrument names as track IDs for InsertChain lookup. The mixer volume/pan commands should use the same scheme: `track.{instrumentName}.volume` for engine-known tracks. For volume/pan on tracks that don't yet have engine-side support, store in Zustand only (engine enforcement can be added when AudioGraph supports dynamic tracks).
**Warning signs:** "Insert chain not found" errors when trying to add effects to a track.

## Code Examples

### RMS Computation in Audio Thread

```cpp
// Compute RMS for a stereo buffer in processBlock
// Called per-track after instrument output, before insert chain
float computeRms(const float* channelData, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += channelData[i] * channelData[i];
    }
    return std::sqrt(sum / static_cast<float>(numSamples));
}

float computePeak(const float* channelData, int numSamples) {
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float abs = std::fabs(channelData[i]);
        if (abs > peak) peak = abs;
    }
    return peak;
}
```

### Bridge GetMeterLevels Endpoint

```cpp
// Returns all meter levels in a single IPC call
Napi::Value GetMeterLevels(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto deferred = Napi::Promise::Deferred::New(env);
    auto tsfn = Napi::ThreadSafeFunction::New(
        env, Napi::Function::New(env, [](const Napi::CallbackInfo&) {}),
        "GetMeterLevels", 0, 1);

    std::thread([deferred, tsfn]() {
        auto& engine = calliope::Engine::getInstance();
        auto levels = engine.getAudioGraph().getMeterLevels();

        tsfn.BlockingCall([deferred, levels](Napi::Env env, Napi::Function) {
            auto result = Napi::Object::New(env);
            // Per-track levels
            for (auto& [trackId, data] : levels.tracks) {
                auto obj = Napi::Object::New(env);
                obj.Set("rmsLeft", data.rmsLeft);
                obj.Set("rmsRight", data.rmsRight);
                obj.Set("peakLeft", data.peakLeft);
                obj.Set("peakRight", data.peakRight);
                result.Set(trackId, obj);
            }
            // Master levels
            auto master = Napi::Object::New(env);
            master.Set("rmsLeft", levels.master.rmsLeft);
            master.Set("rmsRight", levels.master.rmsRight);
            master.Set("peakLeft", levels.master.peakLeft);
            master.Set("peakRight", levels.master.peakRight);
            result.Set("master", master);
            deferred.Resolve(result);
        });
        tsfn.Release();
    }).detach();

    return deferred.Promise();
}
```

### Canvas 2D Level Meter Rendering

```typescript
// LevelMeter.tsx - Canvas 2D stereo meter
function drawMeter(
  ctx: CanvasRenderingContext2D,
  width: number,
  height: number,
  leftLevel: number,  // 0.0 - 1.0
  rightLevel: number,
  leftPeak: number,
  rightPeak: number
) {
  const channelW = (width - 1) / 2  // 1px gap between L and R
  ctx.clearRect(0, 0, width, height)

  // Background
  ctx.fillStyle = '#1a1a2e'
  ctx.fillRect(0, 0, width, height)

  // Draw channel bar (bottom-up)
  for (const [i, level] of [leftLevel, rightLevel].entries()) {
    const x = i * (channelW + 1)
    const barH = level * height

    // Create gradient
    const grad = ctx.createLinearGradient(0, height, 0, 0)
    grad.addColorStop(0.0, '#22c55e')   // green (bottom)
    grad.addColorStop(0.6, '#22c55e')   // green to -12dB
    grad.addColorStop(0.9, '#f59e0b')   // amber to -3dB
    grad.addColorStop(1.0, '#ef4444')   // red at top

    ctx.fillStyle = grad
    ctx.fillRect(x, height - barH, channelW, barH)
  }

  // Peak hold indicators
  for (const [i, peak] of [leftPeak, rightPeak].entries()) {
    const x = i * (channelW + 1)
    const peakY = height - peak * height
    ctx.fillStyle = '#eeeeee'
    ctx.fillRect(x, peakY, channelW, 1)
  }
}
```

### Rotary Pan Knob

```typescript
// PanKnob.tsx - CSS rotation with pointer drag
function PanKnob({ value, onChange }: { value: number; onChange: (v: number) => void }) {
  const knobRef = useRef<HTMLDivElement>(null)
  const dragStartY = useRef(0)
  const dragStartValue = useRef(0)

  const rotation = value * 135  // -1.0 to +1.0 maps to -135deg to +135deg

  const handlePointerDown = (e: React.PointerEvent) => {
    e.currentTarget.setPointerCapture(e.pointerId)
    dragStartY.current = e.clientY
    dragStartValue.current = value
  }

  const handlePointerMove = (e: React.PointerEvent) => {
    if (!e.currentTarget.hasPointerCapture(e.pointerId)) return
    const delta = (dragStartY.current - e.clientY) / 100  // up = right
    const newValue = Math.max(-1, Math.min(1, dragStartValue.current + delta))
    onChange(newValue)
  }

  const handlePointerUp = (e: React.PointerEvent) => {
    e.currentTarget.releasePointerCapture(e.pointerId)
  }

  const handleDoubleClick = () => onChange(0)  // Reset to center

  const label = value === 0 ? 'C' : value < 0 ? `${Math.round(-value * 100)}L` : `${Math.round(value * 100)}R`

  return (
    <div className="flex flex-col items-center">
      <div
        ref={knobRef}
        className="w-8 h-8 rounded-full bg-[#3a3a5a] relative cursor-pointer"
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onDoubleClick={handleDoubleClick}
      >
        <div
          className="absolute top-1 left-1/2 w-0.5 h-3 bg-[#eeeeee] origin-bottom"
          style={{ transform: `translateX(-50%) rotate(${rotation}deg)` }}
        />
      </div>
      <span className="text-[10px] text-[#999999] mt-0.5">{label}</span>
    </div>
  )
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Web Audio API AnalyserNode for meters | C++ RMS/peak via atomics + IPC polling | Project architecture decision | More accurate, lower latency, no Web Audio overhead |
| Redux for mixer state | Zustand with per-track selectors | Project convention | Prevents cascade re-renders when one track's meter updates |
| PixiJS for meters | Canvas 2D | CONTEXT.md decision | Simpler, no WebGL context needed for simple bar rendering |

## Open Questions

1. **Track ID mapping to engine insert chains**
   - What we know: The engine has hardcoded insert chains for "polysynth", "basssynth", "drumMachine", and "master". The UI creates tracks with UUID-based IDs.
   - What's unclear: How to map UI track IDs to engine-recognized track IDs for effect insert operations and meter level reporting.
   - Recommendation: For Phase 8, use the instrument name strings ("polysynth", "basssynth", "drumMachine") as the engine-facing track IDs. The mixer-store should maintain a mapping from UI track ID to engine track ID. Volume/pan for tracks without engine mapping are stored in Zustand only (no engine enforcement until dynamic track support is added).

2. **Volume/pan enforcement in the audio thread**
   - What we know: MasterBusProcessor has `std::atomic<float> masterVolume`. Per-track volume/pan is NOT yet implemented in the C++ audio graph.
   - What's unclear: Where in the audio processing chain to apply per-track volume and pan (before or after insert chain? In the instrument processor? In a new gain/pan node?).
   - Recommendation: Apply volume and pan AFTER the instrument output and BEFORE the insert chain. This matches standard DAW routing: instrument -> gain/pan -> effects -> master. Implement as part of the InsertChainProcessor or as a simple wrapper around each instrument's output in the AudioGraph processBlock.

3. **Per-track volume/pan parameter registration**
   - What we know: The ParameterRegistry supports dynamic registration with `track.{trackId}.volume` scheme per CONTEXT.md.
   - What's unclear: Whether to register these at engine initialization (for hardcoded instruments) or dynamically.
   - Recommendation: Register at engine initialization for the three existing instruments. Use parameter IDs like `track.polysynth.volume` and `track.polysynth.pan`.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Vitest (latest, via project config) |
| Config file | `vitest.config.ts` |
| Quick run command | `npx vitest run --reporter=verbose` |
| Full suite command | `npx vitest run --reporter=verbose` |

### Phase Requirements -> Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MIX-01 | Track type has volume/pan fields; mixer-store tracks panel state; fader dB conversion math | unit | `npx vitest run test/mixer-store.test.ts -x` | No - Wave 0 |
| MIX-01 | setTrackVolume/setTrackPan actions update store and dispatch commands | unit | `npx vitest run test/mixer-store.test.ts -x` | No - Wave 0 |
| MIX-02 | Master strip state management and master volume dispatch | unit | `npx vitest run test/mixer-store.test.ts -x` | No - Wave 0 |
| MIX-03 | Effect chain state tracking in mixer-store (add/remove/reorder/bypass) | unit | `npx vitest run test/mixer-store.test.ts -x` | No - Wave 0 |
| MIX-04 | Meter level update and peak hold decay logic | unit | `npx vitest run test/mixer-helpers.test.ts -x` | No - Wave 0 |
| ALL | dB/gain conversion functions (gainToDb, dbToGain, faderToGain, gainToFader) | unit | `npx vitest run test/mixer-helpers.test.ts -x` | No - Wave 0 |

### Sampling Rate

- **Per task commit:** `npx vitest run --reporter=verbose`
- **Per wave merge:** `npx vitest run --reporter=verbose`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps

- [ ] `test/mixer-store.test.ts` -- mixer store state management, volume/pan actions, effect chain tracking
- [ ] `test/mixer-helpers.test.ts` -- dB/gain conversion, fader position mapping, meter smoothing math

## Sources

### Primary (HIGH confidence)

- Codebase inspection: `engine/include/calliope/audio_graph.h` -- AudioGraph structure, insert chain access
- Codebase inspection: `engine/include/calliope/master_bus.h` -- MasterBusProcessor with atomic masterVolume
- Codebase inspection: `engine/include/calliope/insert_chain.h` -- InsertChain API (insert/remove/reorder/bypass)
- Codebase inspection: `engine/include/calliope/parameter_registry.h` -- ParameterRegistry getter/setter/range
- Codebase inspection: `engine/include/calliope/engine.h` -- Engine singleton, getInsertChain, registerEffectParameters
- Codebase inspection: `app/src/preload/index.ts` -- Existing IPC bridge with effect convenience methods
- Codebase inspection: `app/src/renderer/types/calliope.d.ts` -- CalliopeAPI type definitions
- Codebase inspection: `app/src/renderer/stores/timeline-store.ts` -- Track type (needs volume/pan extension)
- Codebase inspection: `app/src/renderer/components/tracks/TrackHeader.tsx` -- M/S/Arm button pattern
- Codebase inspection: `app/src/renderer/App.tsx` -- Panel layout pattern (timeline + piano roll + split divider)
- Codebase inspection: `app/src/renderer/components/timeline/TimelineToolbar.tsx` -- Toggle button pattern
- Phase context: `.planning/phases/08-mixer/08-CONTEXT.md` -- Locked decisions
- Phase context: `.planning/phases/08-mixer/08-UI-SPEC.md` -- Full UI design contract

### Secondary (MEDIUM confidence)

- `.planning/research/ARCHITECTURE.md` -- getMeterLevels triple-buffer pattern (documented in architecture research but not yet implemented in code)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - all libraries already in project, no new dependencies
- Architecture: HIGH - patterns established in Phases 5-7, direct codebase inspection
- Pitfalls: HIGH - standard real-time audio metering challenges well-documented in DAW development
- Engine metering: MEDIUM - the C++ metering implementation is new code; pattern is standard but exact integration points need validation during implementation

**Research date:** 2026-03-29
**Valid until:** 2026-04-28 (stable domain, no external dependency changes)
