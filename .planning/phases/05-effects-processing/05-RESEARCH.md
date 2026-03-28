# Phase 5: Effects Processing - Research

**Researched:** 2026-03-28
**Domain:** JUCE DSP effects processing, insert chain architecture, C++ audio processing
**Confidence:** HIGH

## Summary

Phase 5 builds five audio effects (parametric EQ, compressor, reverb, delay, limiter) and the insert chain infrastructure that applies them per-track and on the master bus. The excellent news is that JUCE 8's `juce_dsp` module provides production-ready implementations for every required effect: `dsp::IIR::ArrayCoefficients` for EQ bands, `dsp::Compressor` for dynamics, `dsp::Reverb` (wrapping `juce::Reverb`) for algorithmic reverb, `dsp::DelayLine` + `dsp::DryWetMixer` for tempo-synced delay, and `dsp::Limiter` for brick-wall limiting. All of these are already linked via `juce::juce_dsp` in `engine/CMakeLists.txt`.

The primary design challenge is the **InsertChain** abstraction -- a container that holds an ordered list of effect processors and processes audio through them serially. This must be real-time safe (no allocations on the audio thread), support dynamic insert/remove/reorder/bypass, and integrate with the existing AudioProcessorGraph routing. The current architecture routes instruments directly to MasterBusProcessor; Phase 5 must insert per-track effect chains between instruments and the master bus.

**Primary recommendation:** Build each effect as an AudioProcessor subclass (following the established instrument pattern), create an InsertChain class that manages an ordered vector of effect processors with lock-free swapping for real-time safety, and extend the AudioProcessorGraph to route through per-track chains. All effect parameters use atomic members registered with ParameterRegistry, and all operations go through CommandDispatcher.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FX-01 | Parametric EQ with at least 4 bands (low shelf, 2 parametric, high shelf) | JUCE `dsp::IIR::ArrayCoefficients` provides `makeLowShelf`, `makePeakFilter`, `makeHighShelf` -- all verified in source. 4 bands = 4 IIR filters processed in series per channel. |
| FX-02 | Dynamic compressor with threshold, ratio, attack, release, makeup gain | JUCE `dsp::Compressor<float>` provides `setThreshold`, `setRatio`, `setAttack`, `setRelease`. Makeup gain is a simple `dsp::Gain` applied after compression. |
| FX-03 | Algorithmic reverb with room size, damping, wet/dry, pre-delay | JUCE `dsp::Reverb` wraps `juce::Reverb` with `Parameters{roomSize, damping, wetLevel, dryLevel, width, freezeMode}`. Pre-delay via short `dsp::DelayLine` before reverb input. |
| FX-04 | Tempo-synced delay with feedback, wet/dry, and ping-pong option | JUCE `dsp::DelayLine<float, Linear>` for delay buffer, `dsp::DryWetMixer` for mix. Tempo sync calculated from Transport BPM. Ping-pong = alternating L/R feedback. |
| FX-05 | Brick-wall limiter on master bus for final output loudness control | JUCE `dsp::Limiter<float>` provides `setThreshold` and `setRelease`. Two-stage compressor + hard clip at 0dB. Verified in JUCE source. |
| FX-06 | Each effect exposes all parameters via the command dispatcher interface | Follow established pattern: atomic members on processor, registered in ParameterRegistry with getter/setter lambdas, SetParameterCommand for undoable changes. |
| ENG-03 | Audio routing supports per-track insert effect chains with serial processing | InsertChain class managing ordered effect processors. AudioProcessorGraph rerouted: instrument -> InsertChainProcessor -> MasterBus. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE juce_dsp | 8.0.12 | All DSP primitives (IIR filters, Compressor, Limiter, Reverb, DelayLine, DryWetMixer, Gain) | Already linked in CMakeLists.txt. Industry standard for DAW development. Every required effect has a JUCE primitive. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| dsp::IIR::ArrayCoefficients | (part of juce_dsp) | Parametric EQ filter coefficient calculation | makeLowShelf, makePeakFilter, makeHighShelf for EQ bands |
| dsp::IIR::Filter | (part of juce_dsp) | Per-channel IIR filter processing | One filter instance per channel per EQ band |
| dsp::Compressor<float> | (part of juce_dsp) | Dynamic range compression | Threshold/ratio/attack/release compressor |
| dsp::Limiter<float> | (part of juce_dsp) | Brick-wall limiting | Two-stage compressor + hard clip at 0dB |
| dsp::Reverb | (part of juce_dsp) | Algorithmic reverb (wraps juce::Reverb) | Freeverb-style reverb with room/damping/wet/dry |
| dsp::DelayLine<float, Linear> | (part of juce_dsp) | Delay buffer with interpolation | pushSample/popSample API for feedback delay |
| dsp::DryWetMixer<float> | (part of juce_dsp) | Dry/wet mixing with latency compensation | Reverb and delay wet/dry control |
| dsp::Gain<float> | (part of juce_dsp) | Gain stage | Makeup gain for compressor |
| dsp::ProcessSpec | (part of juce_dsp) | DSP initialization specification | Passed to prepare() on all DSP objects |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| JUCE dsp::Reverb | Custom Schroeder/Freeverb | JUCE Reverb IS a Freeverb implementation. No reason to hand-roll. |
| JUCE dsp::Compressor | Hand-rolled RMS/peak compressor | JUCE version handles ballistics correctly. Custom only needed for sidechain (v2). |
| Per-processor IIR filters | dsp::ProcessorChain | ProcessorChain is statically typed (template). Dynamic insert chain needs runtime polymorphism. |

**Installation:** No additional packages needed -- all JUCE DSP modules already linked.

## Architecture Patterns

### Recommended Project Structure
```
engine/
├── include/calliope/
│   ├── effects/
│   │   ├── parametric_eq.h          # ParametricEqProcessor
│   │   ├── compressor.h             # CompressorProcessor
│   │   ├── reverb.h                 # ReverbProcessor
│   │   ├── delay.h                  # DelayProcessor
│   │   └── limiter.h                # LimiterProcessor
│   ├── insert_chain.h               # InsertChain (manages ordered effects)
│   └── commands/
│       └── effect_commands.h         # InsertEffect, RemoveEffect, ReorderEffect, BypassEffect
├── src/
│   ├── effects/
│   │   ├── parametric_eq.cpp
│   │   ├── compressor.cpp
│   │   ├── reverb.cpp
│   │   ├── delay.cpp
│   │   └── limiter.cpp
│   ├── insert_chain.cpp
│   └── commands/
│       └── effect_commands.cpp
```

### Pattern 1: Effect Processor (AudioProcessor Subclass)
**What:** Each effect is an `AudioProcessor` subclass with atomic parameters, matching the instrument pattern from Phase 4.
**When to use:** Every effect implementation.
**Example:**
```cpp
// Source: Verified from poly_synth.h pattern + JUCE dsp::Compressor API
class CompressorProcessor : public juce::AudioProcessor {
public:
    CompressorProcessor();

    // Standard AudioProcessor overrides (same boilerplate as instruments)
    const juce::String getName() const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    // ... editor stubs, getTailLengthSeconds, etc.

    // Atomic parameters (same pattern as PolySynthProcessor)
    std::atomic<float> threshold{-10.0f};   // dB
    std::atomic<float> ratio{4.0f};         // 1:1 to inf:1
    std::atomic<float> attack{10.0f};       // ms
    std::atomic<float> release{100.0f};     // ms
    std::atomic<float> makeupGain{0.0f};    // dB
    std::atomic<bool> bypassed{false};

private:
    juce::dsp::Compressor<float> compressor_;
    juce::dsp::Gain<float> makeup_;
};
```

### Pattern 2: InsertChain (Real-Time Safe Effect Container)
**What:** Manages an ordered list of effect processors that process audio serially. Uses lock-free pointer swapping for real-time safety when the chain is modified.
**When to use:** Per-track and master bus effect routing.
**Example:**
```cpp
// Source: Project pattern (lock-free from Phase 2) + AudioProcessorGraph ownership model
class InsertChain {
public:
    // Called on audio thread -- must be lock-free
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Called on message thread -- may allocate
    int insertEffect(std::unique_ptr<juce::AudioProcessor> effect, int position = -1);
    void removeEffect(int position);
    void moveEffect(int fromPosition, int toPosition);
    void setBypass(int position, bool bypassed);
    bool isBypassed(int position) const;

    int getNumEffects() const;
    juce::AudioProcessor* getEffect(int position) const;

private:
    // Double-buffer pattern: message thread builds new chain,
    // atomically swaps pointer for audio thread
    struct ChainState {
        std::vector<std::unique_ptr<juce::AudioProcessor>> effects;
        std::vector<bool> bypassed;
    };

    // Audio thread reads activeChain_ via atomic pointer
    std::atomic<ChainState*> activeChain_{nullptr};

    // Message thread owns both and swaps
    std::unique_ptr<ChainState> chainA_, chainB_;

    double currentSampleRate_ = 44100.0;
    int currentBlockSize_ = 512;
};
```

### Pattern 3: AudioGraph Routing Change for Per-Track Chains
**What:** Currently instruments connect directly to MasterBusProcessor. Phase 5 inserts an InsertChainProcessor between each instrument and the master bus.
**When to use:** Modifying AudioGraph::initialise() routing.
**Example:**
```
Before (Phase 4):
  PolySynth -> MasterBus -> Output
  BassSynth -> MasterBus -> Output
  DrumMachine -> MasterBus -> Output

After (Phase 5):
  PolySynth -> InsertChainProcessor[polysynth] -> MasterBus -> Output
  BassSynth -> InsertChainProcessor[basssynth] -> MasterBus -> Output
  DrumMachine -> InsertChainProcessor[drumMachine] -> MasterBus -> Output
  MasterBus has its own InsertChain (for limiter)
```

### Pattern 4: Effect Commands (Undoable Chain Operations)
**What:** Commands for inserting, removing, reordering, and bypassing effects in insert chains, following the Command pattern from Phase 3.
**When to use:** All effect chain mutations.
**Example:**
```cpp
// Source: instrument_commands.h pattern
class InsertEffectCommand : public Command {
public:
    InsertEffectCommand(InsertChain& chain, const juce::String& effectType, int position)
        : chain_(chain), effectType_(effectType), position_(position) {}

    bool perform() override;
    bool undo() override;  // removes the effect
    int getSizeInUnits() override { return 1; }
    juce::String getCommandName() const override { return "effect.insert"; }
    juce::var getEventData() const override;
    bool isUndoable() const override { return true; }

private:
    InsertChain& chain_;
    juce::String effectType_;
    int position_;
    // Store removed effect for undo
    std::unique_ptr<juce::AudioProcessor> removedEffect_;
};
```

### Pattern 5: Parametric EQ Implementation
**What:** 4-band EQ using IIR filters: low shelf, 2x peak, high shelf. Each band has frequency, Q, and gain.
**When to use:** FX-01 implementation.
**Example:**
```cpp
// Source: Verified from juce_IIRFilter.h ArrayCoefficients API
class ParametricEqProcessor : public juce::AudioProcessor {
public:
    // Per-band parameters (4 bands)
    struct BandParams {
        std::atomic<float> frequency;
        std::atomic<float> q;
        std::atomic<float> gainDb;
        std::atomic<bool> enabled{true};
    };

    BandParams bands[4]; // [0]=lowShelf, [1]=peak1, [2]=peak2, [3]=highShelf
    std::atomic<bool> bypassed{false};

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        if (bypassed.load(std::memory_order_relaxed)) return;

        // Update coefficients if parameters changed
        updateFilters();

        juce::dsp::AudioBlock<float> block(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);

        for (int band = 0; band < 4; ++band) {
            if (bands[band].enabled.load()) {
                for (int ch = 0; ch < 2; ++ch)
                    filters_[band][ch].process(context);
            }
        }
    }

private:
    // 4 bands x 2 channels = 8 IIR filter instances
    juce::dsp::IIR::Filter<float> filters_[4][2];
    double sampleRate_ = 44100.0;

    void updateFilters() {
        // Band 0: Low shelf
        auto coeffs0 = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
            sampleRate_, bands[0].frequency.load(), bands[0].q.load(),
            juce::Decibels::decibelsToGain(bands[0].gainDb.load()));
        // ... apply to filters_[0][0] and filters_[0][1]

        // Band 1-2: Peak filters
        // Band 3: High shelf
    }
};
```

### Anti-Patterns to Avoid
- **Allocating on the audio thread:** Never use `new`, `std::vector::push_back`, or `std::shared_ptr` reference counting on the audio thread. Use atomic pointer swaps or pre-allocated buffers.
- **Calling setCoefficients every sample:** Update IIR filter coefficients only when parameters change, not every sample. Use a dirty flag or compare previous values.
- **Using dsp::ProcessorChain for the insert chain:** ProcessorChain is compile-time typed via templates. The insert chain needs runtime-dynamic effect ordering -- use a vector of processors instead.
- **Mutex/lock in processBlock:** The audio thread must never block on a mutex. Use atomic pointer swapping or lock-free structures for chain modifications.
- **Forgetting prepareToPlay on dynamically added effects:** When inserting an effect at runtime, it must be prepared with the current sample rate and buffer size before being swapped into the active chain.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Compressor ballistics | Custom envelope follower with attack/release | `dsp::Compressor<float>` | Uses BallisticsFilter internally, handles smoothing correctly |
| Reverb algorithm | Custom Schroeder/Freeverb/plate reverb | `dsp::Reverb` (wraps `juce::Reverb`) | Freeverb implementation, well-tuned, zero bugs |
| Limiter | Custom soft-knee compressor + clipper | `dsp::Limiter<float>` | Two-stage compressor + hard clip at 0dB, proven |
| Delay line interpolation | Custom circular buffer with linear interpolation | `dsp::DelayLine<float, Linear>` | Handles fractional delay, multiple interpolation types |
| Dry/wet mixing | Manual gain calculations with crossfade | `dsp::DryWetMixer<float>` | Handles latency compensation, multiple mixing rules |
| IIR filter coefficients | Custom biquad coefficient calculation | `dsp::IIR::ArrayCoefficients` | Correct implementations of lowShelf, highShelf, peakFilter, bandPass |
| dB to gain conversion | `pow(10, db/20)` | `juce::Decibels::decibelsToGain()` | Handles edge cases, consistent across codebase |

**Key insight:** JUCE's `juce_dsp` module provides production-quality implementations for every effect in this phase. The only custom code needed is the InsertChain container and the parameter wiring.

## Common Pitfalls

### Pitfall 1: IIR Filter Coefficient Update Zipper Noise
**What goes wrong:** Updating IIR filter coefficients abruptly causes audible clicks/zippers.
**Why it happens:** Biquad IIR filters have internal state that becomes discontinuous when coefficients change suddenly.
**How to avoid:** Use `juce::dsp::IIR::Coefficients` (reference-counted, thread-safe) and update via `*filter.coefficients = *newCoeffs`. JUCE IIR::Filter handles coefficient interpolation when using the ProcessContext interface. Alternatively, smooth parameter changes with `juce::SmoothedValue` before computing coefficients.
**Warning signs:** Clicks when sweeping EQ frequency in real-time.

### Pitfall 2: Reverb Thread Safety
**What goes wrong:** Calling `setParameters()` on `juce::Reverb` while `processStereo()` is running causes artifacts.
**Why it happens:** JUCE's Reverb documentation explicitly states: "this doesn't attempt to lock the reverb."
**How to avoid:** Copy atomic parameter values to a local `Parameters` struct at the start of processBlock, then call `setParameters()` immediately before `process()` within the same audio callback. This is safe because both happen on the audio thread sequentially.
**Warning signs:** Intermittent crackles when changing reverb parameters during playback.

### Pitfall 3: DelayLine Maximum Size Not Set
**What goes wrong:** `dsp::DelayLine` silently clips delay times to its maximum size.
**Why it happens:** Default constructor creates a very small delay line. Must call `setMaximumDelayInSamples()` before `prepare()` or use the constructor variant.
**How to avoid:** Calculate maximum delay from BPM range: at 20 BPM, a whole note = 12 seconds. At 48kHz = 576000 samples. Allocate at least this much. Use `DelayLine(maxSamples)` constructor.
**Warning signs:** Delay time seems capped regardless of parameter setting.

### Pitfall 4: InsertChain Modification During Processing
**What goes wrong:** Removing or reordering effects while the audio thread is iterating causes use-after-free or wrong processing order.
**Why it happens:** The audio thread reads the chain while the message thread modifies it.
**How to avoid:** Double-buffer pattern: message thread builds a new chain state in the inactive buffer, then atomically swaps the pointer. Old chain kept alive until next swap (prevents dangling pointers).
**Warning signs:** Crashes when adding/removing effects during playback.

### Pitfall 5: Forgetting to Prepare Dynamically Added Effects
**What goes wrong:** Effect produces silence or crashes on first processBlock call.
**Why it happens:** JUCE DSP objects require `prepare(ProcessSpec)` before processing. Dynamically inserted effects miss this step.
**How to avoid:** InsertChain must call `prepareToPlay(sampleRate, blockSize)` on every newly created effect before swapping it into the active chain.
**Warning signs:** Silence from newly inserted effect, or assertion failure in debug build.

### Pitfall 6: Per-Channel IIR Filter State
**What goes wrong:** Stereo EQ sounds wrong or has phase issues.
**Why it happens:** A single `dsp::IIR::Filter` instance processes one channel. Using one filter for both channels corrupts internal state.
**How to avoid:** Create one filter instance per channel per band. For 4-band stereo EQ = 8 filter instances.
**Warning signs:** One channel sounds filtered differently, or mono collapses produce artifacts.

### Pitfall 7: Delay Feedback > 1.0
**What goes wrong:** Delay output grows to infinity, producing extremely loud noise.
**Why it happens:** Feedback coefficient > 1.0 causes exponential growth in the delay buffer.
**How to avoid:** Clamp feedback to [0.0, 0.98] maximum. Document this in parameter registration (max value).
**Warning signs:** Output volume increasing over time with delay active.

## Code Examples

### Parametric EQ Band Update (Verified from JUCE source)
```cpp
// Source: juce_dsp/processors/juce_IIRFilter.h ArrayCoefficients API
void ParametricEqProcessor::updateBandCoefficients(int bandIndex) {
    float freq = bands[bandIndex].frequency.load(std::memory_order_relaxed);
    float q = bands[bandIndex].q.load(std::memory_order_relaxed);
    float gainDb = bands[bandIndex].gainDb.load(std::memory_order_relaxed);
    float gainFactor = juce::Decibels::decibelsToGain(gainDb);

    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> newCoeffs;

    switch (bandIndex) {
        case 0: // Low shelf
            newCoeffs = new juce::dsp::IIR::Coefficients<float>(
                juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
                    sampleRate_, freq, q, gainFactor));
            break;
        case 1: // Peak 1
        case 2: // Peak 2
            newCoeffs = new juce::dsp::IIR::Coefficients<float>(
                juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
                    sampleRate_, freq, q, gainFactor));
            break;
        case 3: // High shelf
            newCoeffs = new juce::dsp::IIR::Coefficients<float>(
                juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
                    sampleRate_, freq, q, gainFactor));
            break;
    }

    for (int ch = 0; ch < 2; ++ch)
        *filters_[bandIndex][ch].coefficients = *newCoeffs;
}
```

### Compressor processBlock (Verified from JUCE source)
```cpp
// Source: juce_dsp/widgets/juce_Compressor.h API
void CompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (bypassed.load(std::memory_order_relaxed)) return;

    // Update parameters from atomics (audio thread safe -- these are simple setters)
    compressor_.setThreshold(threshold.load(std::memory_order_relaxed));
    compressor_.setRatio(ratio.load(std::memory_order_relaxed));
    compressor_.setAttack(attack.load(std::memory_order_relaxed));
    compressor_.setRelease(release.load(std::memory_order_relaxed));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor_.process(context);

    // Makeup gain
    float makeupDb = makeupGain.load(std::memory_order_relaxed);
    if (makeupDb != 0.0f) {
        float gain = juce::Decibels::decibelsToGain(makeupDb);
        buffer.applyGain(gain);
    }
}
```

### Tempo-Synced Delay Calculation
```cpp
// Source: Standard DAW tempo-sync calculation
float DelayProcessor::getDelayTimeInSamples() const {
    // Note values: 1/4=1.0, 1/8=0.5, 1/16=0.25, dotted=1.5x, triplet=2/3x
    float bpm = currentBpm_.load(std::memory_order_relaxed);
    float noteValue = syncNoteValue_.load(std::memory_order_relaxed);

    float beatsPerSecond = bpm / 60.0f;
    float delaySeconds = noteValue / beatsPerSecond;

    return delaySeconds * static_cast<float>(sampleRate_);
}

// Ping-pong: on each feedback iteration, swap L/R channels
void DelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (bypassed.load(std::memory_order_relaxed)) return;

    float delaySamples = getDelayTimeInSamples();
    float fb = feedback.load(std::memory_order_relaxed);
    bool pingPong = pingPongEnabled.load(std::memory_order_relaxed);

    dryWetMixer_.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float input = buffer.getSample(ch, sample);
            float delayed = delayLines_[ch].popSample(ch, delaySamples);

            float feedbackSample = delayed * fb;
            if (pingPong) {
                // Cross-feed: L feedback goes to R delay, R to L
                int otherCh = 1 - ch;
                delayLines_[otherCh].pushSample(otherCh, input + feedbackSample);
            } else {
                delayLines_[ch].pushSample(ch, input + feedbackSample);
            }

            buffer.setSample(ch, sample, delayed);
        }
    }

    dryWetMixer_.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}
```

### InsertChain processBlock (Lock-Free Pattern)
```cpp
// Source: Project pattern from Phase 2 lock-free queue + audio graph ownership
void InsertChain::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    auto* chain = activeChain_.load(std::memory_order_acquire);
    if (!chain) return;

    for (size_t i = 0; i < chain->effects.size(); ++i) {
        if (!chain->bypassed[i] && chain->effects[i]) {
            chain->effects[i]->processBlock(buffer, midi);
        }
    }
}
```

### Parameter Registration Pattern (from Engine::registerParameters)
```cpp
// Source: engine.cpp registerParameters() pattern
void Engine::registerEffectParameters() {
    // Example: Compressor on track "polysynth"
    // Parameter ID format: "effects.<trackId>.<slotIndex>.<paramName>"
    // e.g., "effects.polysynth.0.threshold"

    // This is dynamic -- effects are registered when inserted and
    // unregistered when removed. The InsertChain triggers registration.
}
```

### Bridge Command Extension Pattern
```cpp
// Source: bridge.cpp DispatchCommand pattern
// In the command dispatch switch:
} else if (command == "effect.insert") {
    // effectType: "eq", "compressor", "reverb", "delay", "limiter"
    // trackId: "polysynth", "basssynth", "drumMachine", "master"
    // position: insert slot index (-1 for end)
    cmd = std::make_unique<calliope::InsertEffectCommand>(
        resolveInsertChain(trackId), effectType, position);
} else if (command == "effect.remove") {
    cmd = std::make_unique<calliope::RemoveEffectCommand>(
        resolveInsertChain(trackId), position);
} else if (command == "effect.reorder") {
    cmd = std::make_unique<calliope::ReorderEffectCommand>(
        resolveInsertChain(trackId), fromPosition, toPosition);
} else if (command == "effect.bypass") {
    cmd = std::make_unique<calliope::BypassEffectCommand>(
        resolveInsertChain(trackId), position, bypassed);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| juce::IIRFilter (old API) | dsp::IIR::Filter<float> | JUCE 5+ | New DSP module has better API, ProcessContext integration |
| Manual dry/wet mixing | dsp::DryWetMixer | JUCE 6+ | Handles latency compensation, multiple mixing rules |
| Projucer build system | CMake (already using) | JUCE 6+ | No impact, already on CMake |

**Deprecated/outdated:**
- `juce::IIRFilter` (old namespace): Use `juce::dsp::IIR::Filter<float>` instead
- `juce::Reverb` direct usage: Wrap with `juce::dsp::Reverb` for ProcessContext integration

## Open Questions

1. **Dynamic Parameter IDs for Inserted Effects**
   - What we know: Current parameters are registered at engine init time with fixed IDs. Effects are inserted dynamically at runtime.
   - What's unclear: Should parameter IDs be registered when effects are inserted and unregistered when removed? Or should we pre-register slots?
   - Recommendation: Register on insert, unregister on remove. Use a naming convention like `effects.<trackId>.<slotIndex>.<paramName>`. The ParameterRegistry already supports dynamic add via `registerParameter()`. Need to add a `removeParameter()` method.

2. **InsertChain as AudioProcessor vs Standalone Class**
   - What we know: AudioProcessorGraph expects AudioProcessor nodes. An InsertChain could either be an AudioProcessor that wraps effects, or a standalone class called directly.
   - What's unclear: Whether to add InsertChainProcessor nodes to the graph, or modify MasterBusProcessor to contain an InsertChain.
   - Recommendation: Create InsertChainProcessor as an AudioProcessor subclass. Insert it into the graph between instruments and master bus. This keeps the graph routing clean and the chain self-contained.

3. **Effect Ownership During Undo**
   - What we know: When an effect is removed, undo should restore it. The Command must hold the removed effect.
   - What's unclear: How to handle the case where the effect processor had accumulated state (e.g., reverb tail).
   - Recommendation: Accept that undo restores the effect with reset state. This is standard DAW behavior. The undone effect gets `prepareToPlay` called again.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 3.x (TypeScript) / Catch2 (C++ - not yet configured) |
| Config file | vitest.config.ts (exists in project) |
| Quick run command | `npx vitest run --reporter=verbose` |
| Full suite command | `npx vitest run` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FX-01 | Parametric EQ processes audio with 4 bands | integration (bridge) | `npx vitest run test/effects.test.ts -t "parametric eq"` | No - Wave 0 |
| FX-02 | Compressor applies dynamic range compression | integration (bridge) | `npx vitest run test/effects.test.ts -t "compressor"` | No - Wave 0 |
| FX-03 | Reverb applies room/damping/wet/dry processing | integration (bridge) | `npx vitest run test/effects.test.ts -t "reverb"` | No - Wave 0 |
| FX-04 | Delay applies tempo-synced delay with feedback | integration (bridge) | `npx vitest run test/effects.test.ts -t "delay"` | No - Wave 0 |
| FX-05 | Limiter limits output at threshold | integration (bridge) | `npx vitest run test/effects.test.ts -t "limiter"` | No - Wave 0 |
| FX-06 | Effect parameters controllable via dispatcher | integration (bridge) | `npx vitest run test/effects.test.ts -t "parameter"` | No - Wave 0 |
| ENG-03 | Per-track insert chains with serial processing | integration (bridge) | `npx vitest run test/effects.test.ts -t "insert chain"` | No - Wave 0 |

### Sampling Rate
- **Per task commit:** `npx vitest run test/effects.test.ts --reporter=verbose`
- **Per wave merge:** `npx vitest run`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `test/effects.test.ts` -- covers FX-01 through FX-06 and ENG-03
- [ ] Test approach: Bridge-level tests dispatching effect commands and verifying parameter registration. Audio processing correctness is verified by build + manual listening (DSP correctness is JUCE's responsibility).

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.12 source code (vendored at `vendor/JUCE/modules/juce_dsp/`) -- all DSP class APIs verified by reading headers directly
- `juce_dsp/widgets/juce_Compressor.h` -- Compressor API: setThreshold, setRatio, setAttack, setRelease
- `juce_dsp/widgets/juce_Limiter.h` -- Limiter API: setThreshold, setRelease, two-stage + hard clip
- `juce_dsp/widgets/juce_Reverb.h` -- dsp::Reverb wrapping juce::Reverb with ProcessContext
- `juce_audio_basics/utilities/juce_Reverb.h` -- Reverb::Parameters: roomSize, damping, wetLevel, dryLevel, width, freezeMode
- `juce_dsp/processors/juce_DelayLine.h` -- DelayLine API: setDelay, pushSample, popSample
- `juce_dsp/processors/juce_DryWetMixer.h` -- DryWetMixer API: pushDrySamples, mixWetSamples, setWetMixProportion
- `juce_dsp/processors/juce_IIRFilter.h` -- IIR::ArrayCoefficients: makeLowShelf, makeHighShelf, makePeakFilter
- Existing codebase: engine.cpp, audio_graph.cpp, master_bus.cpp, bridge.cpp, poly_synth.h (patterns)

### Secondary (MEDIUM confidence)
- None needed -- all findings from direct source inspection

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All JUCE DSP classes verified by reading vendored source headers directly
- Architecture: HIGH - Insert chain pattern follows established AudioProcessor pattern from Phases 2-4, with well-understood lock-free techniques
- Pitfalls: HIGH - All pitfalls derived from direct JUCE API documentation and known audio programming patterns

**Research date:** 2026-03-28
**Valid until:** 2027-03-28 (JUCE DSP module is stable, no breaking changes expected)
