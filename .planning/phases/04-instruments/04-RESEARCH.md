# Phase 4: Instruments - Research

**Researched:** 2026-03-28
**Domain:** JUCE synthesizer DSP, polyphonic voice management, sample playback
**Confidence:** HIGH

## Summary

Phase 4 builds three instrument processors (PolySynth, BassSynth, DrumMachine/Sampler) on top of the existing AudioProcessorGraph, CommandDispatcher, and ParameterRegistry infrastructure from Phases 2-3. The implementation is straightforward because JUCE provides all the building blocks: `juce::Synthesiser` for polyphonic voice management, `juce::ADSR` for envelopes, `juce::dsp::LadderFilter` for filters, `juce::dsp::Oscillator` for waveform generation, and `juce::SamplerSound`/`juce::SamplerVoice` for sample playback.

The existing codebase establishes clear patterns: each processor is an `AudioProcessor` subclass with atomic parameters, registered in `ParameterRegistry` with string IDs, controllable via `SetParameterCommand` through the `CommandDispatcher`, and connected to the master bus in `AudioGraph`. The metronome processor serves as a direct template. The main complexity is in the DSP: implementing proper ADSR envelopes per-voice, bandlimited oscillators, filter modulation, and LFO routing -- all real-time safe on the audio thread.

**Primary recommendation:** Use JUCE's built-in `Synthesiser`/`SynthesiserVoice` architecture for polyphony management, `juce::dsp::Oscillator` with bandlimited waveforms, `juce::dsp::LadderFilter` for the filter, `juce::ADSR` for envelopes, and `juce::SamplerSound`/`juce::SamplerVoice` for the drum machine. Each instrument is an `AudioProcessor` wrapping a `Synthesiser` instance.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None -- all implementation choices are at Claude's discretion (infrastructure phase).

### Claude's Discretion
All implementation choices are at Claude's discretion -- pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key considerations from Phase 1-3 patterns:
- Instruments are AudioProcessor subclasses that plug into the AudioProcessorGraph
- Each instrument registers its parameters with ParameterRegistry for ID-based control
- Instrument commands go through CommandDispatcher (undoable parameter changes)
- JUCE juce_dsp module provides oscillators, filters, envelopes (already linked)
- JUCE juce_audio_formats provides WAV/MP3 reading for sampler
- Bridge pattern extends with instrument-specific commands

### Deferred Ideas (OUT OF SCOPE)
None.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| INST-01 | Subtractive/wavetable polysynth with oscillators, filter, envelopes, and LFO | JUCE Synthesiser + SynthesiserVoice for polyphony, dsp::Oscillator for waveforms, dsp::LadderFilter for filter, ADSR for envelopes, custom LFO via dsp::Oscillator at sub-audio rates |
| INST-02 | Bass synthesizer optimized for low-frequency sounds with sub-oscillator | Monophonic or low-polyphony Synthesiser with dual oscillator (main + sub one octave below), heavier LadderFilter LPF24 mode |
| INST-03 | Sample-based drum machine / sampler that loads WAV/MP3 samples and maps to pads/keys | JUCE SamplerSound + SamplerVoice + AudioFormatManager for file loading, MIDI note-to-pad mapping |
| INST-04 | Each instrument exposes all parameters via the command dispatcher interface | Existing ParameterRegistry + SetParameterCommand pattern from Phase 3, atomic parameters on processors |
</phase_requirements>

## Standard Stack

### Core (Already in Project)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE juce_audio_basics | 8.0.12 | Synthesiser, SynthesiserVoice, SynthesiserSound, ADSR, MidiBuffer | Built-in polyphonic voice allocator with MIDI note management. No external dependency. |
| JUCE juce_dsp | 8.0.12 | dsp::Oscillator, dsp::LadderFilter, dsp::ProcessorChain, dsp::Gain | Industry standard DSP primitives for synth building blocks |
| JUCE juce_audio_formats | 8.0.12 | AudioFormatManager, AudioFormatReader, SamplerSound, SamplerVoice | WAV/MP3/AIFF sample loading built into JUCE |
| JUCE juce_audio_processors | 8.0.12 | AudioProcessor base class | Already used by MetronomeProcessor and MasterBusProcessor |

### Supporting (Already Linked)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| JUCE juce_core | 8.0.12 | File I/O, JSON, threading | Loading sample files from disk |
| node-addon-api | 8.5.0 | Bridge instrument commands to JS | Instrument bridge functions |

### No New Dependencies Required
This phase requires zero new npm or system-level dependencies. All JUCE modules are already linked in `engine/CMakeLists.txt`. The `juce_audio_formats` module includes `SamplerSound`/`SamplerVoice` classes.

## Architecture Patterns

### Recommended Project Structure
```
engine/
  include/calliope/
    instruments/
      poly_synth.h           # PolySynthProcessor (AudioProcessor wrapping Synthesiser)
      poly_synth_voice.h     # PolySynthVoice (SynthesiserVoice subclass)
      poly_synth_sound.h     # PolySynthSound (SynthesiserSound subclass)
      bass_synth.h           # BassSynthProcessor
      bass_synth_voice.h     # BassSynthVoice
      bass_synth_sound.h     # BassSynthSound
      drum_machine.h         # DrumMachineProcessor
    commands/
      instrument_commands.h  # CreateInstrument, LoadSample commands
  src/
    instruments/
      poly_synth.cpp
      poly_synth_voice.cpp
      bass_synth.cpp
      bass_synth_voice.cpp
      drum_machine.cpp
    commands/
      instrument_commands.cpp
  tests/
    test_poly_synth.cpp
    test_bass_synth.cpp
    test_drum_machine.cpp
    test_instrument_commands.cpp
```

### Pattern 1: Instrument as AudioProcessor Wrapping Synthesiser
**What:** Each instrument is an `AudioProcessor` subclass that owns a `juce::Synthesiser` instance. The `processBlock()` method clears the buffer and calls `synthesiser.renderNextBlock()`. Atomic parameters on the processor are read by voices during rendering.
**When to use:** All three instruments follow this pattern.
**Example:**
```cpp
// Source: JUCE Synthesiser tutorial + existing MetronomeProcessor pattern
class PolySynthProcessor : public juce::AudioProcessor {
public:
    PolySynthProcessor()
        : AudioProcessor(BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo()))
    {
        // Add voices for polyphony (16 voices)
        for (int i = 0; i < 16; ++i)
            synth_.addVoice(new PolySynthVoice(*this));

        // Add the sound definition
        synth_.addSound(new PolySynthSound());
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        synth_.setCurrentPlaybackSampleRate(sampleRate);
        // Prepare each voice's DSP chain
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
        buffer.clear();
        synth_.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    }

    // Atomic parameters readable by voices on audio thread
    std::atomic<float> oscMix{0.5f};       // osc1/osc2 balance
    std::atomic<int> osc1Waveform{0};      // 0=saw, 1=square, 2=sine, 3=triangle
    std::atomic<int> osc2Waveform{0};
    std::atomic<float> filterCutoff{1000.0f};
    std::atomic<float> filterResonance{0.5f};
    std::atomic<float> filterEnvAmount{0.5f};
    std::atomic<float> ampAttack{0.01f};
    std::atomic<float> ampDecay{0.1f};
    std::atomic<float> ampSustain{0.8f};
    std::atomic<float> ampRelease{0.3f};
    std::atomic<float> filterAttack{0.01f};
    std::atomic<float> filterDecay{0.2f};
    std::atomic<float> filterSustain{0.5f};
    std::atomic<float> filterRelease{0.3f};
    std::atomic<float> lfoRate{1.0f};      // Hz
    std::atomic<float> lfoDepth{0.0f};
    std::atomic<int> lfoTarget{0};         // 0=pitch, 1=filter, 2=amp
    std::atomic<float> masterGain{0.7f};

private:
    juce::Synthesiser synth_;
};
```

### Pattern 2: Voice Reads Parameters from Parent Processor
**What:** Each `SynthesiserVoice` holds a reference to its parent processor and reads atomic parameters directly during `renderNextBlock()`. This is real-time safe because `std::atomic` with `memory_order_relaxed` is lock-free.
**When to use:** All voice implementations.
**Example:**
```cpp
// Source: Existing atomic parameter pattern from MetronomeProcessor
class PolySynthVoice : public juce::SynthesiserVoice {
public:
    PolySynthVoice(PolySynthProcessor& parent) : parent_(parent) {}

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int) override
    {
        // Read current ADSR params from parent atomics
        juce::ADSR::Parameters ampParams;
        ampParams.attack  = parent_.ampAttack.load(std::memory_order_relaxed);
        ampParams.decay   = parent_.ampDecay.load(std::memory_order_relaxed);
        ampParams.sustain = parent_.ampSustain.load(std::memory_order_relaxed);
        ampParams.release = parent_.ampRelease.load(std::memory_order_relaxed);
        ampEnvelope_.setParameters(ampParams);
        ampEnvelope_.noteOn();

        // Set oscillator frequency from MIDI note
        double freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        osc1Phase_ = 0.0;
        currentFrequency_ = freq;
        level_ = velocity;
    }

    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) {
            ampEnvelope_.noteOff();
        } else {
            ampEnvelope_.reset();
            clearCurrentNote();
        }
    }

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override {
        // Per-sample DSP: oscillator -> filter -> amp envelope
        for (int i = startSample; i < startSample + numSamples; ++i) {
            float envValue = ampEnvelope_.getNextSample();
            if (!ampEnvelope_.isActive()) {
                clearCurrentNote();
                break;
            }
            // Generate oscillator sample, apply filter, apply envelope
            float sample = generateOscillator() * envValue * level_;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample(ch, i, sample);
        }
    }

private:
    PolySynthProcessor& parent_;
    juce::ADSR ampEnvelope_;
    juce::ADSR filterEnvelope_;
    double osc1Phase_ = 0.0;
    double currentFrequency_ = 440.0;
    float level_ = 0.0f;
};
```

### Pattern 3: Instrument Lifecycle in AudioGraph and Engine
**What:** `AudioGraph` creates instrument nodes, connects them to master bus. `Engine` registers instrument parameters and manages lifecycle. The same pattern as MetronomeProcessor.
**When to use:** Adding each instrument to the engine.
**Example:**
```cpp
// In AudioGraph -- add instrument node
auto polySynthProc = std::make_unique<PolySynthProcessor>();
polySynthPtr_ = polySynthProc.get();
polySynthNode_ = graph_.addNode(std::move(polySynthProc));

// Connect polysynth -> master (stereo)
graph_.addConnection({{polySynthNode_->nodeID, 0}, {masterNode_->nodeID, 0}});
graph_.addConnection({{polySynthNode_->nodeID, 1}, {masterNode_->nodeID, 1}});

// In Engine::registerParameters()
paramRegistry_.registerParameter("polysynth.filterCutoff", {
    [this]() -> juce::var { return getAudioGraph().getPolySynth().filterCutoff.load(); },
    [this](const juce::var& v) { getAudioGraph().getPolySynth().filterCutoff.store(
        static_cast<float>(static_cast<double>(v))); },
    "float", 20.0, 20000.0
});
```

### Pattern 4: MIDI Routing to Instruments
**What:** MIDI events from the bridge need to reach instrument processors. The `AudioProcessorGraph` handles MIDI routing through connections. A MIDI input node feeds MIDI to instrument nodes. Alternatively, inject MIDI directly into the `Synthesiser` via a method on the processor.
**When to use:** When instruments need to respond to note-on/note-off from the bridge.
**Example:**
```cpp
// Bridge approach: inject MIDI into instrument processor via command
// In bridge.cpp - new command for note events
class NoteOnCommand : public Command {
public:
    NoteOnCommand(PolySynthProcessor& synth, int note, float velocity)
        : synth_(synth), note_(note), velocity_(velocity) {}

    bool perform() override {
        synth_.addMidiEvent(juce::MidiMessage::noteOn(1, note_, velocity_));
        return true;
    }
    bool undo() override { return true; } // note events not undoable
    bool isUndoable() const override { return false; }
    // ...
};

// In PolySynthProcessor - thread-safe MIDI injection
void addMidiEvent(const juce::MidiMessage& msg) {
    // Use lock-free queue or SpinLock-guarded buffer
    const juce::SpinLock::ScopedLockType lock(midiLock_);
    pendingMidi_.addEvent(msg, 0);
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    {
        const juce::SpinLock::ScopedLockType lock(midiLock_);
        midi.addEvents(pendingMidi_, 0, -1, 0);
        pendingMidi_.clear();
    }
    buffer.clear();
    synth_.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}
```

### Pattern 5: Drum Machine Sample Loading
**What:** The drum machine uses `AudioFormatManager` to load WAV/MP3 files and creates `SamplerSound` objects mapped to specific MIDI notes. Each pad maps to a MIDI note number.
**When to use:** INST-03 sampler/drum machine.
**Example:**
```cpp
// In DrumMachineProcessor
void loadSample(int padIndex, const juce::File& file) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats(); // WAV, AIFF
    // MP3 format is already included in juce_audio_formats

    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr) return;

    // Map pad to MIDI note (pad 0 = C1 = MIDI 36, standard GM drum map)
    int midiNote = 36 + padIndex;
    juce::BigInteger noteRange;
    noteRange.setBit(midiNote);

    // SamplerSound takes ownership of reader
    synth_.addSound(new juce::SamplerSound(
        file.getFileNameWithoutExtension(),  // name
        *reader,                              // source
        noteRange,                            // midi notes
        midiNote,                             // root note
        0.01,                                 // attack time
        0.01,                                 // release time
        10.0                                  // max sample length seconds
    ));
    delete reader;
}
```

### Anti-Patterns to Avoid
- **Allocating memory on audio thread:** Never use `new`, `std::vector::push_back()`, or file I/O inside `processBlock()` or `renderNextBlock()`. All sample loading must happen on a non-audio thread, with the loaded data swapped in atomically.
- **Sharing non-atomic state between audio and UI threads:** All parameters readable by voices must be `std::atomic`. This is the established pattern from Phase 2.
- **Using JUCE's ADSR parameter changes during playback incorrectly:** JUCE docs warn that changing ADSR parameters mid-note can cause discontinuities. Read parameters at `startNote()` time, not per-sample. For LFO modulation of filter, modulate the filter directly, not the ADSR.
- **Forgetting to call `synth_.setCurrentPlaybackSampleRate()`:** Must be called in `prepareToPlay()` before any rendering. Voices that don't know the sample rate will produce wrong frequencies.
- **Processing MIDI in processBlock without clearing buffer first:** JUCE convention is `buffer.clear()` then `synth_.renderNextBlock()` which adds to the buffer. Don't skip the clear.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Polyphonic voice allocation | Custom voice pool with note stealing | `juce::Synthesiser` | Handles voice stealing, note-on/off, velocity, sustain pedal. Dozens of edge cases in voice allocation. |
| ADSR envelope generator | Custom envelope state machine | `juce::ADSR` | Sample-accurate, handles edge cases (re-trigger, fast note-off before attack completes). |
| Moog-style ladder filter | Custom filter implementation | `juce::dsp::LadderFilter` | 5 filter modes (LPF12/24, HPF12/24, BPF12), drive/saturation, zero-delay feedback. Correctly handles resonance self-oscillation. |
| Bandlimited oscillators | Naive `sin(phase)` per sample | `juce::dsp::Oscillator` with PolyBLEP or wavetable | Naive sawtooth/square produce aliasing. JUCE's oscillator supports custom functions with proper anti-aliasing. |
| WAV/MP3 file reading | Custom audio decoder | `juce::AudioFormatManager` + `SamplerSound` | Handles bit depth conversion, sample rate mismatch, MP3 decoding, memory management. |
| Sample interpolation for pitch shifting | Linear interpolation of samples | `juce::SamplerVoice` | Built-in cubic interpolation for pitch-shifted playback across the keyboard. |

**Key insight:** JUCE's `juce_audio_basics` and `juce_dsp` modules provide nearly everything needed for a professional synthesizer. The work is in wiring them together correctly, not in implementing DSP algorithms from scratch.

## Common Pitfalls

### Pitfall 1: Oscillator Aliasing
**What goes wrong:** Naive sawtooth/square wave generation produces harmonics above Nyquist that fold back as audible artifacts (aliasing).
**Why it happens:** A mathematical sawtooth has infinite harmonics. At 44.1kHz, notes above ~5kHz will have harmonics that alias.
**How to avoid:** Use bandlimited waveforms. `juce::dsp::Oscillator` accepts a lambda for waveform generation -- use PolyBLEP correction or pre-computed wavetables. For a polysynth, wavetable lookup is most CPU-efficient.
**Warning signs:** Metallic/harsh sound on high notes, frequencies that shouldn't be present in spectrum.

### Pitfall 2: Audio Thread Safety Violations
**What goes wrong:** Calling memory allocation, file I/O, or locking a mutex on the audio thread causes audio dropouts (glitches, clicks).
**Why it happens:** Audio callbacks have strict real-time constraints (~5ms at 44.1kHz/256 buffer). Any operation that could block is forbidden.
**How to avoid:** All parameters via `std::atomic`. Sample loading on a background thread. Use `juce::SpinLock` (not `std::mutex`) for short critical sections. Follow the existing `std::memory_order_relaxed` pattern from Phase 2.
**Warning signs:** Intermittent clicks/pops, especially when changing parameters or loading samples.

### Pitfall 3: Voice Count and CPU Budget
**What goes wrong:** Too many active voices (e.g., 64-voice polyphony with complex DSP per voice) exceeds the CPU budget for the buffer callback.
**Why it happens:** Each voice runs oscillator + filter + envelope per sample. At 44.1kHz with 512 buffer, you have ~11ms for all voices.
**How to avoid:** Default 16 voices for polysynth is reasonable. BassSynth can be monophonic or 4-voice max. Monitor with `juce::AudioProcessorPlayer`'s CPU usage. JUCE Synthesiser handles voice stealing automatically.
**Warning signs:** CPU meter spiking, audio dropouts under heavy polyphony.

### Pitfall 4: Sample Rate Changes
**What goes wrong:** Instruments produce wrong pitches or filters have wrong cutoff after sample rate change.
**Why it happens:** `prepareToPlay()` is called with new sample rate but internal state (oscillator phase, filter coefficients) isn't re-initialized.
**How to avoid:** Reset all DSP state in `prepareToPlay()`. Call `synth_.setCurrentPlaybackSampleRate()`. Re-prepare all voices' dsp chains with new spec.
**Warning signs:** Wrong pitch after changing audio device, filter sounds different at 48kHz vs 44.1kHz.

### Pitfall 5: MIDI Note Mapping for Drum Machine
**What goes wrong:** Drum pads don't trigger or trigger the wrong sample.
**Why it happens:** MIDI note ranges in `SamplerSound` don't match the note numbers being sent from the bridge.
**How to avoid:** Use standard GM drum mapping (kick=36, snare=38, hi-hat=42, etc.) or a simple linear mapping (pad 0 = note 36, pad 1 = note 37, ...). Document the mapping in the API.
**Warning signs:** No sound from certain pads, wrong sample plays.

### Pitfall 6: Sampler Memory Management
**What goes wrong:** Loading large samples causes memory pressure or crashes.
**Why it happens:** `SamplerSound` loads the entire audio file into memory. A 10-second stereo 44.1kHz file is ~3.4MB.
**How to avoid:** Limit maximum sample length (e.g., 30 seconds). For a drum machine, samples are typically short (< 5 seconds). Validate file size before loading.
**Warning signs:** Memory usage climbing with each sample load, OOM on 32-bit builds.

## Code Examples

### Bandlimited Sawtooth via PolyBLEP
```cpp
// Source: Standard PolyBLEP technique for bandlimited waveforms
// Use with juce::dsp::Oscillator's custom function
float polyBlep(float t, float dt) {
    // t = phase [0,1), dt = phase increment per sample
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// In voice: generate bandlimited saw
float generateSaw(double phase, double phaseIncrement) {
    float t = static_cast<float>(phase);
    float dt = static_cast<float>(phaseIncrement);
    float naive = 2.0f * t - 1.0f;  // naive saw: -1 to 1
    return naive - polyBlep(t, dt);  // subtract PolyBLEP correction
}
```

### LFO Implementation
```cpp
// Source: Standard LFO pattern for synth modulation
class LFO {
public:
    void prepare(double sampleRate) { sampleRate_ = sampleRate; }

    void setRate(float hz) { rate_ = hz; }
    void setDepth(float depth) { depth_ = depth; } // 0.0 - 1.0

    float getNextSample() {
        float value = std::sin(2.0f * juce::MathConstants<float>::pi * phase_);
        phase_ += rate_ / static_cast<float>(sampleRate_);
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        return value * depth_;
    }

    void reset() { phase_ = 0.0f; }

private:
    double sampleRate_ = 44100.0;
    float phase_ = 0.0f;
    float rate_ = 1.0f;
    float depth_ = 0.0f;
};
```

### Parameter Registration Pattern for Instruments
```cpp
// Source: Existing Engine::registerParameters() pattern
// In Engine::registerParameters() -- add after existing params
auto& polySynth = getAudioGraph().getPolySynth();

paramRegistry_.registerParameter("polysynth.osc1Waveform", {
    [&polySynth]() -> juce::var { return polySynth.osc1Waveform.load(); },
    [&polySynth](const juce::var& v) { polySynth.osc1Waveform.store(static_cast<int>(v)); },
    "int", 0, 3  // 0=saw, 1=square, 2=sine, 3=triangle
});

paramRegistry_.registerParameter("polysynth.filterCutoff", {
    [&polySynth]() -> juce::var { return polySynth.filterCutoff.load(); },
    [&polySynth](const juce::var& v) { polySynth.filterCutoff.store(
        static_cast<float>(static_cast<double>(v))); },
    "float", 20.0, 20000.0
});
// ... repeat for all parameters
```

### Bridge Command Extension for Note Events
```cpp
// Source: Existing bridge.cpp DispatchCommand pattern
// Add to command name mapping in DispatchCommand thread lambda:
} else if (command == "instrument.noteOn") {
    // Extract instrument name, note, velocity from params
    // Route to correct instrument processor
    auto& polySynth = engine.getAudioGraph().getPolySynth();
    polySynth.addMidiEvent(
        juce::MidiMessage::noteOn(1, midiNote, velocity));
    // Note events are non-undoable, return success directly
} else if (command == "instrument.noteOff") {
    auto& polySynth = engine.getAudioGraph().getPolySynth();
    polySynth.addMidiEvent(
        juce::MidiMessage::noteOff(1, midiNote));
} else if (command == "drumMachine.loadSample") {
    auto& drums = engine.getAudioGraph().getDrumMachine();
    drums.loadSample(padIndex, juce::File(filePath));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Raw sin() oscillators | Bandlimited (PolyBLEP/wavetable) | Standard since ~2010 | Eliminates aliasing, professional sound quality |
| Custom voice allocator | JUCE Synthesiser class | JUCE 3+ (2013+) | Handles all polyphony edge cases |
| Biquad filters for synths | dsp::LadderFilter (zero-delay feedback) | JUCE 5+ (2017+) | More musical resonance, no instability at high resonance |
| Per-voice filter instances | Per-voice filter (correct approach) | Always | Each voice needs independent filter state for proper envelope modulation |

**Deprecated/outdated:**
- `juce::dsp::StateVariableFilter` (deprecated in JUCE 7+, replaced by `juce::dsp::StateVariableTPTFilter`)
- JUCE's `Projucer` build system (replaced by CMake, already using CMake)

## Open Questions

1. **Instrument Instance Management**
   - What we know: Current architecture has one instance of each instrument hardcoded in AudioGraph (like MetronomeProcessor). Phase 4 requirements say "load built-in synthesizers" suggesting multiple instances may be needed later.
   - What's unclear: Whether Phase 4 should support creating multiple instrument instances per type, or just one of each.
   - Recommendation: Start with one instance of each (PolySynth, BassSynth, DrumMachine) hardcoded in AudioGraph, matching the MetronomeProcessor pattern. Multi-track with instrument-per-track is a Phase 6 (timeline) concern. Design the classes to be instantiable multiple times but wire up one each for now.

2. **MIDI Routing Architecture**
   - What we know: The bridge currently has no MIDI input path. Instruments need MIDI note-on/off to produce sound.
   - What's unclear: Whether to use the AudioProcessorGraph's MIDI routing or inject MIDI directly into each instrument processor.
   - Recommendation: Inject MIDI directly via a thread-safe pending buffer on each instrument processor (SpinLock + MidiBuffer). This is simpler than adding a MIDI graph IO node and avoids routing complexity. Note commands come through the bridge as `instrument.noteOn` / `instrument.noteOff`.

3. **Wavetable vs PolyBLEP for Oscillators**
   - What we know: Both approaches produce bandlimited waveforms. PolyBLEP is simpler to implement. Wavetable is more CPU-efficient for many voices.
   - What's unclear: Whether 16-voice polyphony with PolyBLEP will fit in CPU budget.
   - Recommendation: Start with PolyBLEP (simpler, fewer files). If CPU profiling shows issues, migrate to wavetable. The voice interface abstracts the oscillator implementation.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 v3.7.1 |
| Config file | engine/tests/CMakeLists.txt |
| Quick run command | `cd /Users/deebarmaly/Calliope/build && ctest --test-dir engine/tests -R "PolySynth\|BassSynth\|DrumMachine\|Instrument" --output-on-failure` |
| Full suite command | `cd /Users/deebarmaly/Calliope/build && ctest --test-dir engine/tests --output-on-failure` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| INST-01 | PolySynth produces sound with oscillators, filter, envelopes, LFO | unit | `ctest -R "PolySynth"` | No -- Wave 0 |
| INST-02 | BassSynth produces low-frequency sound with sub-oscillator | unit | `ctest -R "BassSynth"` | No -- Wave 0 |
| INST-03 | DrumMachine loads WAV/MP3, maps to pads | unit | `ctest -R "DrumMachine"` | No -- Wave 0 |
| INST-04 | All instrument parameters controllable via dispatcher | unit | `ctest -R "Instrument"` | No -- Wave 0 |

### Sampling Rate
- **Per task commit:** `cd /Users/deebarmaly/Calliope/build && cmake --build . --target calliope_engine_tests && ctest --test-dir engine/tests --output-on-failure`
- **Per wave merge:** Full suite above
- **Phase gate:** Full suite green before verification

### Wave 0 Gaps
- [ ] `engine/tests/test_poly_synth.cpp` -- covers INST-01 (voice produces non-zero audio, ADSR shapes output, filter modifies spectrum, LFO modulates target)
- [ ] `engine/tests/test_bass_synth.cpp` -- covers INST-02 (sub-oscillator present, low-frequency output, monophonic behavior)
- [ ] `engine/tests/test_drum_machine.cpp` -- covers INST-03 (loads WAV file, maps to MIDI note, produces audio on trigger)
- [ ] `engine/tests/test_instrument_commands.cpp` -- covers INST-04 (parameter set via dispatcher, undo/redo works, all params registered)
- [ ] Add test files to `engine/tests/CMakeLists.txt`

## Sources

### Primary (HIGH confidence)
- [JUCE Synthesiser Class Reference](https://docs.juce.com/master/classjuce_1_1Synthesiser.html) - Voice allocation, renderNextBlock API
- [JUCE SynthesiserVoice Class Reference](https://docs.juce.com/master/classjuce_1_1SynthesiserVoice.html) - startNote, stopNote, renderNextBlock per-voice API
- [JUCE ADSR Class Reference](https://docs.juce.com/master/classjuce_1_1ADSR.html) - Envelope parameters, noteOn/noteOff, getNextSample
- [JUCE dsp::LadderFilter Class Reference](https://docs.juce.com/master/classdsp_1_1LadderFilter.html) - Filter modes (LPF12/24, HPF12/24, BPF12), cutoff, resonance, drive
- [JUCE SamplerSound Class Reference](https://docs.juce.com/master/classjuce_1_1SamplerSound.html) - Sample loading from AudioFormatReader, MIDI note mapping
- [JUCE Introduction to DSP Tutorial](https://juce.com/tutorials/tutorial_dsp_introduction/) - dsp::ProcessorChain, dsp::Oscillator, dsp::LadderFilter usage
- [JUCE Build MIDI Synthesiser Tutorial](https://juce.com/tutorials/tutorial_synth_using_midi_input/) - Synthesiser + AudioProcessor integration pattern
- Existing codebase: MetronomeProcessor, MasterBusProcessor, CommandDispatcher, ParameterRegistry, bridge.cpp patterns

### Secondary (MEDIUM confidence)
- [JUCE dsp::Oscillator source](https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/widgets/juce_Oscillator.h) - Custom waveform function support
- [JUCE SamplerPluginDemo](https://github.com/WeAreROLI/JUCE/blob/master/examples/Plugins/SamplerPluginDemo.h) - Complete sampler example

### Tertiary (LOW confidence)
- PolyBLEP technique for bandlimited oscillators (well-established DSP technique, multiple academic sources)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All libraries are already linked in the project, JUCE APIs are stable and well-documented
- Architecture: HIGH - Directly extends established patterns from Phases 2-3 (AudioProcessor, atomic params, CommandDispatcher, ParameterRegistry)
- Pitfalls: HIGH - Well-known audio DSP concerns (aliasing, real-time safety, voice management) documented in JUCE tutorials and community
- DSP implementation: MEDIUM - PolyBLEP oscillator and LFO implementation are straightforward but untested in this specific project context

**Research date:** 2026-03-28
**Valid until:** 2026-04-28 (JUCE 8 API stable, no breaking changes expected)
