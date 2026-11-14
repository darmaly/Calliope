// End-to-end tests of the track model: a track created at runtime owns an
// instrument and a mixer channel, plays the clips assigned to it, and is what
// export renders. These need an audio device and are skipped without one.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/engine.h"
#include "calliope/audio_exporter.h"
#include "calliope/commands/effect_commands.h"
#include <chrono>
#include <cmath>
#include <thread>

using namespace calliope;

namespace {

struct EngineSession {
    Engine& engine = Engine::getInstance();
    bool ready = false;

    EngineSession() { ready = engine.initialise(44100.0, 512); }
    ~EngineSession() { engine.shutdown(); }
};

#define REQUIRE_AUDIO_DEVICE(session) \
    if (!(session).ready) SKIP("no audio output device available")

MidiClipData makeClip(const std::string& clipId, const std::string& trackId,
                      double startBeat, double lengthBeats,
                      std::vector<MidiNoteData> notes)
{
    MidiClipData clip;
    clip.clipId = clipId;
    clip.trackId = trackId;
    clip.startBeat = startBeat;
    clip.lengthBeats = lengthBeats;
    clip.notes = std::move(notes);
    return clip;
}

float peakOf(const juce::AudioBuffer<float>& buffer, int channel)
{
    return buffer.getNumSamples() == 0 ? 0.0f
                                       : buffer.getMagnitude(channel, 0, buffer.getNumSamples());
}

float peakOf(const juce::AudioBuffer<float>& buffer)
{
    return std::max(peakOf(buffer, 0), peakOf(buffer, 1));
}

float rmsOf(const juce::AudioBuffer<float>& buffer)
{
    return buffer.getNumSamples() == 0 ? 0.0f : buffer.getRMSLevel(0, 0, buffer.getNumSamples());
}

int firstAudibleSample(const juce::AudioBuffer<float>& buffer)
{
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        if (std::fabs(buffer.getSample(0, i)) > 1.0e-5f || std::fabs(buffer.getSample(1, i)) > 1.0e-5f)
            return i;
    return -1;
}

juce::AudioBuffer<float> render(Engine& engine, double beats, const std::string& solo = {})
{
    return engine.getAudioGraph().renderOffline(beats, 0.0, solo, nullptr);
}

void setParameter(Engine& engine, const juce::String& id, const juce::var& value)
{
    const auto* def = engine.getParameterRegistry().getParameter(id);
    REQUIRE(def != nullptr);
    def->setter(value);
}

juce::File tempFile(const juce::String& name)
{
    return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(name);
}

} // namespace

// ---------------------------------------------------------------------------
// Track lifecycle
// ---------------------------------------------------------------------------

TEST_CASE("Engine starts with no tracks", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);

    REQUIRE(session.engine.getAudioGraph().getTrackIds().empty());
    REQUIRE(session.engine.getParameterRegistry().getParameter("master.volume") != nullptr);
}

TEST_CASE("addTrack creates a track for each instrument type", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;

    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    REQUIRE(engine.addTrack("bass", InstrumentTypes::bassSynth));
    REQUIRE(engine.addTrack("drums", InstrumentTypes::drumMachine));

    auto ids = engine.getAudioGraph().getTrackIds();
    REQUIRE(ids == std::vector<std::string>{"lead", "bass", "drums"});
    REQUIRE(engine.getAudioGraph().getTrackInstrumentType("bass") == InstrumentTypes::bassSynth);
    REQUIRE(engine.getAudioGraph().getTrackMidiSink("drums") != nullptr);
}

TEST_CASE("addTrack rejects duplicates, the master ID and unknown instruments", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;

    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    REQUIRE_FALSE(engine.addTrack("lead", InstrumentTypes::bassSynth));
    REQUIRE_FALSE(engine.addTrack("master", InstrumentTypes::polySynth));
    REQUIRE_FALSE(engine.addTrack("", InstrumentTypes::polySynth));
    REQUIRE_FALSE(engine.addTrack("other", "theremin"));
    REQUIRE(engine.getAudioGraph().getTrackIds().size() == 1);
}

TEST_CASE("Track parameters are registered per track and removed with it", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    auto& registry = engine.getParameterRegistry();

    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    REQUIRE(engine.addTrack("bass", InstrumentTypes::bassSynth));

    REQUIRE(registry.getParameter("track.lead.volume") != nullptr);
    REQUIRE(registry.getParameter("track.lead.pan") != nullptr);
    REQUIRE(registry.getParameter("track.lead.mute") != nullptr);
    REQUIRE(registry.getParameter("track.lead.solo") != nullptr);
    REQUIRE(registry.getParameter("track.lead.filterCutoff") != nullptr);
    REQUIRE(registry.getParameter("track.bass.subOscMix") != nullptr);
    // Each track has its own instrument, so the same setting is independent per track.
    REQUIRE(registry.getParameter("track.lead.subOscMix") == nullptr);

    setParameter(engine, "track.lead.filterCutoff", 1234.0);
    REQUIRE_THAT(static_cast<double>(registry.getParameter("track.lead.filterCutoff")->getter()),
                 Catch::Matchers::WithinAbs(1234.0, 0.01));
    REQUIRE_THAT(static_cast<double>(registry.getParameter("track.bass.filterCutoff")->getter()),
                 Catch::Matchers::WithinAbs(800.0, 0.01));

    REQUIRE(engine.removeTrack("lead"));
    REQUIRE(registry.getParameter("track.lead.volume") == nullptr);
    REQUIRE(registry.getParameter("track.lead.filterCutoff") == nullptr);
    REQUIRE(registry.getParameter("track.bass.volume") != nullptr);
    REQUIRE_FALSE(engine.removeTrack("lead"));
}

TEST_CASE("Parameter setters clamp to their declared range", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));

    setParameter(engine, "track.lead.filterCutoff", 1.0e9);
    REQUIRE_THAT(static_cast<double>(engine.getParameterRegistry().getParameter("track.lead.filterCutoff")->getter()),
                 Catch::Matchers::WithinAbs(20000.0, 0.01));

    setParameter(engine, "track.lead.volume", -5.0);
    REQUIRE(static_cast<double>(engine.getParameterRegistry().getParameter("track.lead.volume")->getter()) == 0.0);

    setParameter(engine, "track.lead.osc1Waveform", 99);
    REQUIRE(static_cast<int>(engine.getParameterRegistry().getParameter("track.lead.osc1Waveform")->getter()) == 3);
}

TEST_CASE("An unknown track has no insert chain", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));

    REQUIRE(engine.findInsertChain("lead") != nullptr);
    REQUIRE(engine.findInsertChain("master") != nullptr);
    REQUIRE(engine.findInsertChain("lead") != engine.findInsertChain("master"));
    // Previously an unknown ID silently resolved to the master chain.
    REQUIRE(engine.findInsertChain("no-such-track") == nullptr);
}

TEST_CASE("Engine can be shut down and initialised again", "[tracks][integration]")
{
    auto& engine = Engine::getInstance();
    if (!engine.initialise(44100.0, 512)) SKIP("no audio output device available");
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    engine.shutdown();
    REQUIRE_FALSE(engine.isInitialised());
    REQUIRE(engine.getParameterRegistry().getAllParameterIds().empty());

    REQUIRE(engine.initialise(44100.0, 512));
    REQUIRE(engine.isInitialised());
    REQUIRE(engine.getAudioGraph().getTrackIds().empty());
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    engine.shutdown();
}

TEST_CASE("Changing the buffer size keeps the tracks", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    setParameter(engine, "track.lead.filterCutoff", 900.0);

    engine.setBufferSize(1024);  // the device may or may not accept the size

    REQUIRE(engine.getAudioGraph().hasTrack("lead"));
    REQUIRE_THAT(static_cast<double>(engine.getParameterRegistry().getParameter("track.lead.filterCutoff")->getter()),
                 Catch::Matchers::WithinAbs(900.0, 0.01));
}

// ---------------------------------------------------------------------------
// Sound: clips on a track are heard through that track
// ---------------------------------------------------------------------------

TEST_CASE("A clip on each instrument type produces sound", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;

    struct Case { const char* instrument; int pitch; };
    for (const auto& c : { Case{InstrumentTypes::polySynth, 60},
                           Case{InstrumentTypes::bassSynth, 36},
                           Case{InstrumentTypes::drumMachine, 36} }) {
        engine.resetProject();
        REQUIRE(engine.addTrack("t", c.instrument));
        engine.getClipScheduler().addClip(makeClip("c", "t", 0.0, 2.0, {{c.pitch, 0.0, 1.0, 0.8f}}));

        auto audio = render(engine, 2.0);

        INFO("instrument " << c.instrument);
        REQUIRE(audio.getNumSamples() > 0);
        REQUIRE(peakOf(audio) > 0.01f);
    }
}

TEST_CASE("An empty project renders silence", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);

    auto audio = render(session.engine, 2.0);

    REQUIRE(audio.getNumSamples() > 0);
    REQUIRE(peakOf(audio) == 0.0f);
}

TEST_CASE("A note on beat 0 is heard from the very first samples", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("drums", InstrumentTypes::drumMachine));
    engine.getClipScheduler().addClip(makeClip("c", "drums", 0.0, 1.0, {{36, 0.0, 0.25, 1.0f}}));

    auto audio = render(engine, 1.0);

    const int first = firstAudibleSample(audio);
    REQUIRE(first >= 0);
    REQUIRE(first < 64);
}

TEST_CASE("A note starts on its exact sample at the device sample rate", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("drums", InstrumentTypes::drumMachine));
    engine.getClipScheduler().addClip(makeClip("c", "drums", 0.0, 4.0, {{36, 1.5, 0.25, 1.0f}}));

    auto audio = render(engine, 4.0);

    // 120 BPM: beat 1.5 is 0.75 seconds in, whatever rate the device runs at.
    const double sampleRate = engine.getAudioGraph().getAudioConfig().sampleRate;
    const int expected = static_cast<int>(std::llround(0.75 * sampleRate));
    const int first = firstAudibleSample(audio);
    REQUIRE(first >= expected);
    REQUIRE(first < expected + 64);
}

TEST_CASE("Clips on a removed track stop playing", "[tracks][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    engine.getClipScheduler().addClip(makeClip("c", "lead", 0.0, 2.0, {{60, 0.0, 1.0, 0.8f}}));
    REQUIRE(peakOf(render(engine, 2.0)) > 0.01f);

    REQUIRE(engine.removeTrack("lead"));

    REQUIRE(engine.getClipScheduler().getClipCount() == 0);
    REQUIRE(peakOf(render(engine, 2.0)) == 0.0f);
}

TEST_CASE("A track added from another thread is heard on the live device", "[tracks][integration][live]")
{
    // The app initialises the engine on a worker thread and then edits it from
    // the JavaScript thread, with no JUCE message loop running. JUCE defers
    // graph rebuilds requested off its message thread to that loop, so this
    // test fails if a track edit ever relies on a deferred rebuild.
    auto& engine = Engine::getInstance();
    bool ready = false;
    std::thread initThread([&] { ready = engine.initialise(44100.0, 512); });
    initThread.join();
    if (!ready) SKIP("no audio output device available");

    engine.setMetronomeEnabled(false);
    REQUIRE(engine.addTrack("live", InstrumentTypes::polySynth));
    engine.getClipScheduler().addClip(makeClip("c", "live", 0.0, 8.0, {{60, 0.0, 8.0, 0.9f}}));

    engine.transportPlay();
    float trackPeak = 0.0f;
    float masterPeak = 0.0f;
    for (int i = 0; i < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        auto levels = engine.getMeterLevels();
        for (const auto& track : levels.tracks)
            trackPeak = std::max(trackPeak, track.peakLeft);
        masterPeak = std::max(masterPeak, levels.master.peakLeft);
    }
    const bool advanced = engine.getTransport().getSamplePosition() > 0;
    engine.transportStop();
    engine.shutdown();

    REQUIRE(advanced);
    REQUIRE(trackPeak > 0.01f);
    REQUIRE(masterPeak > 0.01f);
}

// ---------------------------------------------------------------------------
// Mixer: volume, pan, mute, solo
// ---------------------------------------------------------------------------

TEST_CASE("Track volume scales the track's level", "[tracks][mixer][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("bass", InstrumentTypes::bassSynth));
    engine.getClipScheduler().addClip(makeClip("c", "bass", 0.0, 2.0, {{40, 0.0, 2.0, 0.8f}}));

    // RMS, not peak: the synth's oscillators are free-running, so the exact
    // waveform (and its peak) differs from one render to the next.
    const float full = rmsOf(render(engine, 2.0));
    const float fullAgain = rmsOf(render(engine, 2.0));
    setParameter(engine, "track.bass.volume", 0.5);
    const float half = rmsOf(render(engine, 2.0));
    setParameter(engine, "track.bass.volume", 0.0);
    const float off = peakOf(render(engine, 2.0));

    REQUIRE(full > 0.01f);
    REQUIRE_THAT(fullAgain / full, Catch::Matchers::WithinAbs(1.0, 0.05));
    REQUIRE_THAT(half / full, Catch::Matchers::WithinAbs(0.5, 0.05));
    REQUIRE(off == 0.0f);
}

TEST_CASE("Track pan moves the track between the speakers", "[tracks][mixer][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("bass", InstrumentTypes::bassSynth));
    engine.getClipScheduler().addClip(makeClip("c", "bass", 0.0, 2.0, {{40, 0.0, 2.0, 0.8f}}));

    auto centre = render(engine, 2.0);
    REQUIRE_THAT(peakOf(centre, 0), Catch::Matchers::WithinRel(peakOf(centre, 1), 0.01f));

    setParameter(engine, "track.bass.pan", -1.0);
    auto left = render(engine, 2.0);
    REQUIRE(peakOf(left, 0) > 0.01f);
    REQUIRE(peakOf(left, 1) < 1.0e-4f);

    setParameter(engine, "track.bass.pan", 1.0);
    auto right = render(engine, 2.0);
    REQUIRE(peakOf(right, 0) < 1.0e-4f);
    REQUIRE(peakOf(right, 1) > 0.01f);
}

TEST_CASE("Mute silences a track and solo isolates one", "[tracks][mixer][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("bass", InstrumentTypes::bassSynth));
    REQUIRE(engine.addTrack("silent", InstrumentTypes::polySynth));  // has no clips
    engine.getClipScheduler().addClip(makeClip("c", "bass", 0.0, 2.0, {{40, 0.0, 2.0, 0.8f}}));

    REQUIRE(peakOf(render(engine, 2.0)) > 0.01f);

    setParameter(engine, "track.bass.mute", true);
    REQUIRE(peakOf(render(engine, 2.0)) == 0.0f);
    setParameter(engine, "track.bass.mute", false);
    REQUIRE(peakOf(render(engine, 2.0)) > 0.01f);

    // Soloing the empty track silences the bass...
    setParameter(engine, "track.silent.solo", true);
    REQUIRE(static_cast<bool>(engine.getParameterRegistry().getParameter("track.silent.solo")->getter()));
    REQUIRE(peakOf(render(engine, 2.0)) == 0.0f);

    // ...soloing both brings it back, and clearing solo restores the mix.
    setParameter(engine, "track.bass.solo", true);
    REQUIRE(peakOf(render(engine, 2.0)) > 0.01f);
    setParameter(engine, "track.bass.solo", false);
    setParameter(engine, "track.silent.solo", false);
    REQUIRE(peakOf(render(engine, 2.0)) > 0.01f);
}

TEST_CASE("An effect inserted on a track changes that track only", "[tracks][mixer][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("bass", InstrumentTypes::bassSynth));
    engine.getClipScheduler().addClip(makeClip("c", "bass", 0.0, 1.0, {{40, 0.0, 0.25, 0.8f}}));

    // One beat of music, then listen to what is left a second later.
    auto tailEnergy = [&]() {
        auto audio = engine.getAudioGraph().renderOffline(1.0, 2.0, {}, nullptr);
        const int from = audio.getNumSamples() - static_cast<int>(engine.getAudioGraph().getAudioConfig().sampleRate);
        return audio.getMagnitude(0, from, audio.getNumSamples() - from);
    };
    const float dry = tailEnergy();

    auto* chain = engine.findInsertChain("bass");
    REQUIRE(chain != nullptr);
    InsertEffectCommand insert(*chain, engine, "bass", "reverb");
    REQUIRE(insert.perform());
    setParameter(engine, "effects.bass.0.roomSize", 0.95);
    setParameter(engine, "effects.bass.0.wetLevel", 0.8);

    const float wet = tailEnergy();

    REQUIRE(engine.findInsertChain("master")->getNumEffects() == 0);
    REQUIRE(wet > dry + 1.0e-4f);
}

// ---------------------------------------------------------------------------
// Project state
// ---------------------------------------------------------------------------

TEST_CASE("Project state round-trips through JSON and rebuilds the engine", "[tracks][project_state][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;

    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    REQUIRE(engine.addTrack("drums", InstrumentTypes::drumMachine));
    setParameter(engine, "track.lead.filterCutoff", 777.0);
    setParameter(engine, "track.lead.osc1Waveform", 2);
    setParameter(engine, "track.drums.volume", 0.6);
    setParameter(engine, "track.drums.pan", -0.25);
    setParameter(engine, "track.lead.mute", true);
    engine.setBpm(98.0);
    engine.setLoopRegion(4.0, 12.0, true);

    InsertEffectCommand insertDelay(*engine.findInsertChain("lead"), engine, "lead", "delay");
    REQUIRE(insertDelay.perform());
    InsertEffectCommand insertLimiter(*engine.findInsertChain("master"), engine, "master", "limiter");
    REQUIRE(insertLimiter.perform());
    setParameter(engine, "effects.lead.0.feedback", 0.61);
    setParameter(engine, "effects.master.0.threshold", -3.5);
    engine.findInsertChain("lead")->setBypass(0, true);

    engine.getClipScheduler().addClip(makeClip("clip-1", "lead", 4.0, 8.0,
                                               {{60, 0.0, 1.0, 0.8f}, {67, 1.5, 0.5, 0.4f}}));

    const auto savedJson = engine.getProjectState().toJson();

    // Wipe everything, then rebuild purely from the saved JSON.
    engine.resetProject();
    REQUIRE(engine.getAudioGraph().getTrackIds().empty());
    REQUIRE(engine.findInsertChain("master")->getNumEffects() == 0);
    REQUIRE(engine.getParameterRegistry().getParameter("track.lead.volume") == nullptr);

    ProjectState loaded;
    REQUIRE(loaded.fromJson(savedJson));
    REQUIRE(loaded.tracks.size() == 2);
    REQUIRE(loaded.clips.size() == 1);
    REQUIRE(loaded.clips[0].notes.size() == 2);
    REQUIRE(engine.applyProjectState(loaded));

    REQUIRE(engine.getAudioGraph().getTrackIds() == std::vector<std::string>{"lead", "drums"});
    REQUIRE(engine.getAudioGraph().getTrackInstrumentType("drums") == InstrumentTypes::drumMachine);
    REQUIRE(engine.findInsertChain("lead")->getNumEffects() == 1);
    REQUIRE(engine.findInsertChain("lead")->isBypassed(0));
    REQUIRE(engine.findInsertChain("master")->getNumEffects() == 1);
    REQUIRE(engine.getClipScheduler().getClipCount() == 1);
    REQUIRE(engine.getTransport().getBpm() == 98.0);
    REQUIRE(engine.getTransport().isLooping());

    // The rebuilt engine describes itself exactly as the original did.
    REQUIRE(engine.getProjectState().toJson() == savedJson);
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

TEST_CASE("Exporting the mix writes an audible file", "[tracks][export][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    engine.getClipScheduler().addClip(makeClip("c", "lead", 0.0, 4.0, {{60, 0.0, 2.0, 0.8f}}));

    auto file = tempFile("calliope_test_mix.wav");
    file.deleteFile();

    ExportOptions options;
    options.outputPath = file.getFullPathName().toStdString();
    options.format = ExportFormat::WAV_24;

    float lastProgress = -1.0f;
    REQUIRE(engine.getAudioExporter().exportMix(options, [&](float p) { lastProgress = p; }));
    REQUIRE(lastProgress == 1.0f);
    REQUIRE(file.existsAsFile());

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    REQUIRE(reader != nullptr);
    REQUIRE(reader->numChannels == 2);
    REQUIRE(reader->bitsPerSample == 24);

    // Four beats at 120 BPM plus the two-second tail.
    const double seconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    REQUIRE_THAT(seconds, Catch::Matchers::WithinAbs(4.0, 0.01));

    juce::AudioBuffer<float> audio(2, static_cast<int>(reader->lengthInSamples));
    reader->read(&audio, 0, audio.getNumSamples(), 0, true, true);
    REQUIRE(peakOf(audio) > 0.01f);

    reader.reset();
    file.deleteFile();

    // The device is handed back afterwards and the transport is left stopped.
    REQUIRE(engine.getTransport().getState() == TransportState::Stopped);
    REQUIRE(engine.getTransport().getSamplePosition() == 0);
}

TEST_CASE("Exporting with nothing to render fails instead of writing silence", "[tracks][export][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);

    auto file = tempFile("calliope_test_empty.wav");
    file.deleteFile();
    ExportOptions options;
    options.outputPath = file.getFullPathName().toStdString();

    REQUIRE_FALSE(session.engine.getAudioExporter().exportMix(options));
    REQUIRE_FALSE(file.existsAsFile());
}

TEST_CASE("MP3 export reports honestly whether it is available", "[tracks][export][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("lead", InstrumentTypes::polySynth));
    engine.getClipScheduler().addClip(makeClip("c", "lead", 0.0, 1.0, {{60, 0.0, 1.0, 0.8f}}));

    auto file = tempFile("calliope_test.mp3");
    file.deleteFile();
    ExportOptions options;
    options.outputPath = file.getFullPathName().toStdString();
    options.format = ExportFormat::MP3;

    const bool exported = engine.getAudioExporter().exportMix(options);
    REQUIRE(exported == AudioExporter::isFormatSupported(ExportFormat::MP3));
    REQUIRE(file.existsAsFile() == exported);
    file.deleteFile();
}

TEST_CASE("Exporting stems writes one file per track containing only that track", "[tracks][export][integration]")
{
    EngineSession session;
    REQUIRE_AUDIO_DEVICE(session);
    auto& engine = session.engine;
    REQUIRE(engine.addTrack("id-lead", InstrumentTypes::polySynth));
    REQUIRE(engine.addTrack("id-drums", InstrumentTypes::drumMachine));
    REQUIRE(engine.addTrack("id-empty", InstrumentTypes::bassSynth));
    engine.getClipScheduler().addClip(makeClip("c1", "id-lead", 0.0, 2.0, {{60, 0.0, 1.0, 0.8f}}));
    engine.getClipScheduler().addClip(makeClip("c2", "id-drums", 0.0, 2.0, {{36, 0.0, 0.5, 1.0f}}));
    setParameter(engine, "track.id-lead.solo", true);  // must not leak into the stems

    auto dir = tempFile("calliope_test_stems");
    dir.deleteRecursively();

    StemExportOptions options;
    options.outputDir = dir.getFullPathName().toStdString();
    options.trackNames = { {"id-lead", "Lead"}, {"id-drums", "Lead"}, {"id-empty", "Sub: Bass?"} };

    REQUIRE(engine.getAudioExporter().exportStems(options));

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto peakOfFile = [&](const juce::File& f) {
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(f));
        REQUIRE(reader != nullptr);
        juce::AudioBuffer<float> audio(2, static_cast<int>(reader->lengthInSamples));
        reader->read(&audio, 0, audio.getNumSamples(), 0, true, true);
        return peakOf(audio);
    };

    // Duplicate names do not overwrite each other; illegal characters are removed.
    REQUIRE(dir.getNumberOfChildFiles(juce::File::findFiles, "*.wav") == 3);
    REQUIRE(peakOfFile(dir.getChildFile("Lead.wav")) > 0.01f);
    REQUIRE(peakOfFile(dir.getChildFile("Lead 2.wav")) > 0.01f);
    REQUIRE(peakOfFile(dir.getChildFile("Sub Bass.wav")) == 0.0f);

    // The solo state the user had set survives the export.
    REQUIRE(engine.getAudioGraph().isTrackSoloed("id-lead"));

    dir.deleteRecursively();
}
