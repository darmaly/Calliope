# Feature Landscape

**Domain:** AI-powered digital audio workstation (AI agent operates a real DAW via natural language)
**Researched:** 2026-03-27
**Overall Confidence:** MEDIUM-HIGH

## Context

LuneyTunes occupies a unique intersection: it is a real DAW (like FL Studio, Ableton, Logic Pro) that is operated primarily by AI agents through natural language (like Suno, Soundverse, Producer.ai). The target user has musical taste but finds traditional DAWs overwhelming. This shapes every feature decision -- the AI is the primary interface, not a bolt-on assistant.

The competitive landscape splits into three camps:
1. **Traditional DAWs** adding AI features (FL Studio Gopher, Logic Pro Session Players, Ableton stem separation)
2. **AI generators** adding DAW-like controls (Suno Studio multitrack timeline, Udio stem downloads)
3. **AI-native tools** bridging the gap (MIDI Agent plugin, Soundverse Agent, Producer.ai)

LuneyTunes is none of these. It is a standalone DAW where the AI has full programmatic control over every subsystem -- not a plugin, not a generator, not a chatbot bolted onto a manual.

---

## Table Stakes

Features users expect from any music production tool in 2026. Missing any of these means users leave immediately or never adopt.

### Core DAW Engine

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Multi-track timeline/arrangement view | Every DAW has this; it is how music is organized | High | Horizontal timeline with tracks, clips, drag/drop |
| Piano roll / MIDI editor | Standard for writing melodies, chords, drums | High | Note input, velocity, quantize, snap-to-grid |
| Mixer with per-track controls | Volume, pan, mute, solo -- fundamental mixing | Medium | Channel strips, master bus, send/return routing |
| Transport controls (play, stop, record, loop) | Basic playback is non-negotiable | Low | With BPM/tempo control and time signature |
| Audio rendering / bounce / export | Users need to get their music out | Medium | WAV, MP3, FLAC at minimum; stem export |
| Project save / load / autosave | Data loss is unacceptable | Medium | Autosave every N minutes, crash recovery |
| Undo / redo with history | Users (and AI) will make mistakes | Medium | Deep undo stack; critical for AI experimentation |
| Built-in synthesizers (subtractive, wavetable) | Users cannot install VSTs in v1; must ship with usable sounds | High | At least one versatile polysynth and one bass synth |
| Built-in sampler / sample playback | Drums and sample-based music require this | Medium | Load WAV/MP3 samples, map to keys/pads |
| Built-in effects (EQ, compressor, reverb, delay) | Mixing is impossible without these | High | Per-track insert chain; at minimum: parametric EQ, compressor, reverb, delay, limiter |
| Tempo and time signature management | Music has tempo; must be settable and automatable | Low | Global BPM, time signature, tempo automation |
| Metronome / click track | Recording and composition require timing reference | Low | Toggleable, adjustable volume |

### AI-Specific Table Stakes (2026 Expectations)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Natural language chat interface | This is the core product promise | Medium | Persistent chat panel, conversation history |
| AI generates MIDI from text descriptions | MIDI Agent and similar tools have set this expectation since 2025 | High | "Write a jazzy chord progression in Cm" produces MIDI on a track |
| AI selects and loads instruments/presets | Non-technical users cannot browse synth parameters | Medium | AI maps descriptions ("warm pad", "punchy bass") to presets |
| AI applies and configures effects | Users describe desired sound ("more reverb", "tighter low end") | Medium | AI translates to specific parameter changes |
| AI adjusts mixer levels | "Make the drums louder" is a basic expectation | Low | AI reads mixer state and adjusts faders/pans |
| AI explains what it is doing | Users need to learn and build trust | Low | AI narrates actions: "I added a compressor to tighten the bass" |
| Conversational iteration | Users refine through dialogue, not forms | Medium | "Make it darker" / "Undo that" / "Try something else" |
| Full DAW state visibility to AI | AI must "see" the project to make intelligent decisions | High | Structured representation of all tracks, clips, settings, mixer state |

### User Experience Table Stakes

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Visual waveform display | Users need to see audio clips on the timeline | Medium | Rendered waveforms for audio clips |
| Track color coding and naming | Organization in any multi-track environment | Low | AI should auto-name tracks meaningfully |
| Solo / mute per track | Standard mixing workflow | Low | Visual toggle, AI can control |
| Keyboard shortcuts | Power users expect them even in simplified tools | Low | Standard shortcuts for transport, undo, save |
| Responsive UI at audio-engine scale | Clicks, drags, and playback must feel instant | High | Audio engine in C++/JUCE; UI must not block on AI calls |

---

## Differentiators

Features that set LuneyTunes apart from both traditional DAWs and AI generators. Not expected, but valued -- these are the reasons users choose this product.

### Core Differentiator: AI as Primary Producer

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| "Describe a song, get a produced track" end-to-end workflow | The killer feature -- go from empty project to finished song via conversation | Very High | AI orchestrates: create tracks, choose instruments, write MIDI, arrange, mix, master |
| AI-driven arrangement and song structure | AI builds intro/verse/chorus/bridge structure from description | High | Not just loops -- full song arrangement with transitions |
| AI-driven mixing chain | "Make this sound radio-ready" triggers multi-step mix process | High | AI applies EQ, compression, spatial effects, level balancing across all tracks |
| AI-driven mastering | One-click or one-prompt mastering like LANDR but integrated | High | Limiter, EQ, stereo imaging, loudness targeting |
| Genre-aware production templates | "Start a lo-fi hip hop beat" sets up appropriate tracks, instruments, tempo | Medium | AI preloads genre-appropriate instruments, BPM, effects |
| Multiple AI "takes" / alternatives | AI generates 3 variations, user picks favorite | Medium | Similar to Suno generating multiple options; prevents single-path lock-in |

### AI Intelligence Features

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Context-aware suggestions | AI proactively suggests next steps ("Your verse needs a bass line") | Medium | Requires AI to analyze current project state and identify gaps |
| Musical theory awareness | AI understands keys, scales, chord functions -- user does not have to | High | AI writes in correct key, suggests harmonically interesting progressions |
| Reference track matching | "Make it sound like [genre/vibe]" with AI analyzing and matching characteristics | High | Tonal balance, arrangement density, instrument choices |
| Natural language sound design | "I want a sound like a detuned saw wave with slow filter sweep" produces a synth preset | Very High | AI maps descriptions to synthesizer parameters programmatically |
| Iterative refinement with memory | AI remembers entire conversation and project history for coherent iterations | Medium | Not stateless -- tracks what user has liked/disliked across session |

### Production Quality Features

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| AI stem separation (built-in) | Import any audio, AI separates into stems for remixing | High | ML model (like Demucs); every major DAW now ships this |
| AI audio-to-MIDI transcription | Hum or play a melody, get MIDI | High | Audio analysis to note detection; MIDI Agent does this via plugin |
| AI-assisted automation | "Fade the reverb in during the chorus" generates automation curves | Medium | AI writes automation data to parameter lanes |
| Smart quantization and humanization | AI quantizes MIDI but preserves groove / adds human feel | Medium | Not just snap-to-grid -- intelligent timing adjustment |

### User Experience Differentiators

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Guided onboarding via AI conversation | First-run experience is a conversation, not a tutorial | Low | "What kind of music do you want to make?" starts the workflow |
| Progressive complexity disclosure | Start simple, reveal advanced features as user grows | Medium | Beginner sees chat + timeline; advanced users access piano roll, mixer details |
| Visual feedback of AI actions | See the AI "working" -- notes appearing, faders moving | Medium | Builds trust and teaches DAW concepts simultaneously |
| Project "story" / decision log | Scrollable history of what AI did and why | Low | Helps users understand their project and learn production |

---

## Anti-Features

Features to explicitly NOT build. These are tempting but wrong for LuneyTunes.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Fully autonomous "push button, get song" with no interaction | Competes with Suno/Udio on their turf and loses; no user agency; not a DAW | Conversational co-creation -- user guides, AI executes. The conversation IS the product |
| VST/AU plugin hosting (v1) | Enormous complexity (sandboxing, compatibility, crashes); AI cannot programmatically control arbitrary plugins | Ship excellent built-in instruments the AI fully controls; defer plugin hosting to v2+ |
| Complex manual-first UI rivaling Ableton/Logic | Target users are overwhelmed by traditional DAWs; this defeats the purpose | AI-first interface with optional manual controls that progressively reveal |
| Audio recording from microphone (v1) | Adds hardware dependency, latency management, monitoring complexity | Support importing audio files; add live recording in v2 |
| Collaboration / cloud features (v1) | Multi-user sync is a product in itself; distracts from core AI-DAW experience | Single-user local app; export and share via files |
| Music theory lessons / educational content | Not a teaching tool; the AI IS the teacher through action | AI explains what it does in context rather than offering standalone lessons |
| Social features / sharing / community | Scope creep; focus on production quality | Export to standard formats; users share via existing platforms |
| Subscription-gated AI (hosting your own models) | BYOK keeps infrastructure simple, costs on user, and model-agnostic | BYOK architecture; user picks Claude, GPT, Gemini, etc. |
| Real-time collaborative AI (multiple agents) | Coordination complexity; diminishing returns | Single AI agent with full DAW access; user has one conversation partner |
| Mobile version (v1) | DAW UI on mobile is compromised; audio engine constraints | Desktop-first; Electron covers macOS/Windows/Linux |
| Notation/score view | Target users do not read sheet music; adds significant complexity | Piano roll is sufficient; notation is a v2+ consideration |
| Video sync / film scoring | Entirely different workflow and user base | Music-only; defer film/game audio to v2+ |

---

## Feature Dependencies

Critical ordering constraints -- these features cannot be built out of sequence.

```
Audio Engine (C++/JUCE)
  |-> Built-in Synthesizers (depends on audio engine)
  |-> Built-in Effects (depends on audio engine)
  |-> Built-in Sampler (depends on audio engine)
  |-> Audio Rendering/Export (depends on audio engine)
  |-> Mixer (depends on audio engine routing)

MIDI Subsystem
  |-> Piano Roll Editor (depends on MIDI subsystem)
  |-> MIDI Playback through Instruments (depends on MIDI + synths)

Timeline/Arrangement
  |-> Clip Management (depends on timeline)
  |-> Track Management (depends on timeline + mixer)

DAW Command API (Node ↔ C++ bridge)
  |-> AI Integration Layer (depends on command API)
    |-> Natural Language Chat (depends on AI layer)
    |-> AI MIDI Generation (depends on AI layer + MIDI subsystem)
    |-> AI Instrument Selection (depends on AI layer + synth presets)
    |-> AI Mixing (depends on AI layer + mixer)
    |-> AI Arrangement (depends on AI layer + timeline + all instruments)
    |-> AI Mastering (depends on AI layer + effects chain)

Project System
  |-> Save/Load (depends on serializable DAW state)
  |-> Undo/Redo (depends on command pattern in DAW state)
  |-> Autosave (depends on save system)

AI State Representation
  |-> Full DAW State Visibility (depends on all subsystems exposing state)
  |-> Context-Aware Suggestions (depends on state visibility + AI layer)
```

### Dependency Summary

The fundamental chain is:
1. **Audio engine** must exist before anything makes sound
2. **Instruments and effects** must exist before music can be created
3. **Timeline and mixer** must exist before arrangement is possible
4. **DAW command API** (the bridge) must exist before AI can operate anything
5. **AI integration** layer connects to the command API
6. **End-to-end AI workflows** require all of the above working together

---

## MVP Recommendation

### Must Ship (Phase 1-2 Priority)

1. **Audio engine with playback, rendering, and basic routing** -- nothing works without this
2. **At least 2 built-in synthesizers + 1 sampler** -- AI needs instruments to work with
3. **Core effects: EQ, compressor, reverb, delay, limiter** -- minimum viable mixing
4. **Timeline with track/clip management** -- visual arrangement
5. **Piano roll for MIDI editing** -- manual fallback and AI writes here
6. **Mixer with per-track volume/pan/mute/solo** -- basic mixing
7. **DAW command API (C++ to Node bridge)** -- the critical integration layer
8. **AI chat interface with BYOK key management** -- the core interaction model
9. **AI can: create tracks, load instruments, write MIDI, adjust mixer, apply effects** -- the minimum "AI operates DAW" feature set
10. **Project save/load and undo/redo** -- data safety
11. **Export to WAV/MP3** -- users need output

### Defer to Post-MVP

- Stem separation (nice but not core to creation workflow)
- Audio-to-MIDI transcription (same -- enhancement, not foundation)
- AI mastering chain (basic limiter on master is sufficient for MVP)
- Reference track matching (requires significant ML work)
- Natural language sound design (preset selection is sufficient for MVP)
- Multiple AI takes/alternatives (single generation with undo is sufficient)
- Progressive complexity disclosure (ship with one coherent UI level first)
- Automation lanes (manual automation; AI can set static values initially)

### Deferred Indefinitely (Out of Scope for v1)

- VST/AU hosting
- Audio recording from microphone
- Collaboration features
- Mobile version
- Notation view
- Video sync

---

## Competitive Positioning Map

| Capability | Traditional DAWs | AI Generators | LuneyTunes Target |
|-----------|-----------------|---------------|-------------------|
| Full DAW engine | Yes | No | Yes |
| AI-first interface | No (bolt-on) | Yes | Yes |
| Natural language control | Limited (FL Gopher = manual helper) | Yes (but no real DAW) | Yes (controls real DAW) |
| MIDI editing precision | Yes | No | Yes (AI + manual) |
| Sound design control | Yes (manual) | No (black box) | Yes (AI-operated) |
| Mixing/mastering | Yes (manual) | Auto (no control) | Yes (AI-driven, tunable) |
| Instrument-level control | Yes | No | Yes |
| Beginner accessible | No | Yes | Yes |
| Professional output quality | Yes | Improving | Yes (real DSP engine) |

LuneyTunes wins where neither camp fully delivers: **professional DAW capabilities accessible through natural language**, with full transparency and control over every element.

---

## Sources

- [Top AI DAWs in 2025 - Pitch Innovations](https://pitchinnovations.com/blog/top-ai-daws-in-2025-the-future-of-music-production-workflow/)
- [Suno Studio Introduction](https://suno.com/blog/suno-studio)
- [Suno Studio Review - MusicTech](https://musictech.com/reviews/digital-audio-workstations/suno-studio-review/)
- [RipX DAW - AI Stem Separation](https://hitnmix.com/ripx-daw/)
- [BandLab AI Tools](https://www.bandlab.com/products/ai-tools)
- [MIDI Agent VST Plugin](https://www.midiagent.com/)
- [Producer.ai - AI Music Agent](https://www.producer.ai/)
- [AI Music Agents Explained - Soundverse](https://www.soundverse.ai/blog/article/ai-music-agents-explained)
- [Best AI Music Generators 2026 - Superprompt](https://superprompt.com/blog/best-ai-music-generators)
- [iZotope Ozone 12](https://www.izotope.com/en/products/ozone)
- [LANDR Mastering](https://blog.landr.com/best-mastering-plugins/)
- [Generative Audio Workstations - AudioCipher](https://www.audiocipher.com/post/generative-audio-workstation)
- [FL Studio 2025 Review - MusicTech](https://musictech.com/reviews/digital-audio-workstations/fl-studio-2025-review/)
- [Ableton Live vs FL Studio - EDMProd](https://www.edmprod.com/ableton-vs-fl-studio/)
- [AI Sound Design in 2026 - Soundverse](https://www.soundverse.ai/blog/article/how-ai-is-used-in-audio-and-sound-design-0802)
- [The DAW Feature Chart - Admiral Bumblebee](https://www.admiralbumblebee.com/DAW-Explain.html)
