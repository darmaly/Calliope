#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/clip_scheduler.h"
#include "calliope/instruments/poly_synth.h"
#include <vector>

using namespace calliope;

namespace {

// 120 BPM at 44.1 kHz: one beat is exactly 22050 samples.
constexpr double kSampleRate = 44100.0;
constexpr double kBpm = 120.0;
constexpr int kSamplesPerBeat = 22050;

struct RecordingSink : MidiSink {
    struct Event {
        juce::MidiMessage message;
        int offset;
    };
    std::vector<Event> events;

    void addMidiEvent(const juce::MidiMessage& msg, int sampleOffset) override
    {
        events.push_back({msg, sampleOffset});
    }

    int count(bool noteOn) const
    {
        int n = 0;
        for (const auto& e : events)
            if (noteOn ? e.message.isNoteOn() : e.message.isNoteOff()) ++n;
        return n;
    }

    int countAllNotesOff() const
    {
        int n = 0;
        for (const auto& e : events)
            if (e.message.isAllNotesOff()) ++n;
        return n;
    }
};

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

void play(ClipScheduler& scheduler, int numSamples, int64_t startSample,
          bool looping = false, double loopStart = 0.0, double loopEnd = 0.0)
{
    scheduler.processBlock(numSamples, kSampleRate, kBpm, startSample, true,
                           looping, loopStart, loopEnd);
}

bool hasNonZeroSamples(const juce::AudioBuffer<float>& buffer, int channel = 0)
{
    auto* data = buffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        if (data[i] != 0.0f) return true;
    return false;
}

} // namespace

TEST_CASE("ClipScheduler stores, counts and removes clips", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    REQUIRE(scheduler.getClipCount() == 0);

    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));
    scheduler.addClip(makeClip("clip-2", "b", 4.0, 4.0, {}));
    REQUIRE(scheduler.getClipCount() == 2);
    REQUIRE(scheduler.getEndBeat() == 8.0);

    scheduler.removeClip("clip-1");
    REQUIRE(scheduler.getClipCount() == 1);

    scheduler.clearClips();
    REQUIRE(scheduler.getClipCount() == 0);
    REQUIRE(scheduler.getEndBeat() == 0.0);
}

TEST_CASE("ClipScheduler removeClipsForTrack removes only that track's clips", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {}));
    scheduler.addClip(makeClip("clip-2", "a", 4.0, 4.0, {}));
    scheduler.addClip(makeClip("clip-3", "b", 0.0, 4.0, {}));

    scheduler.removeClipsForTrack("a");

    auto remaining = scheduler.getClips();
    REQUIRE(remaining.size() == 1);
    REQUIRE(remaining[0].clipId == "clip-3");
}

TEST_CASE("ClipScheduler plays a note that starts on beat 0", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));

    play(scheduler, 512, 0);

    REQUIRE(sink.count(true) == 1);
    REQUIRE(sink.events[0].offset == 0);
    REQUIRE(sink.events[0].message.getNoteNumber() == 60);
    REQUIRE_THAT(sink.events[0].message.getFloatVelocity(), Catch::Matchers::WithinAbs(0.8, 0.01));
}

TEST_CASE("ClipScheduler places events on their exact sample", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    // Clip at beat 2, note half a beat in, a quarter of a beat long.
    scheduler.addClip(makeClip("clip-1", "a", 2.0, 4.0, {{64, 0.5, 0.25, 1.0f}}));

    const int64_t noteOnSample = static_cast<int64_t>(2.5 * kSamplesPerBeat);    // 55125
    const int64_t noteOffSample = static_cast<int64_t>(2.75 * kSamplesPerBeat);  // 60637 (.5 rounds)

    // Walk the transport in 512-sample blocks across the whole note.
    std::vector<int64_t> onPositions, offPositions;
    for (int64_t position = 0; position < 3 * kSamplesPerBeat; position += 512) {
        sink.events.clear();
        play(scheduler, 512, position);
        for (const auto& e : sink.events) {
            REQUIRE(e.offset >= 0);
            REQUIRE(e.offset < 512);
            if (e.message.isNoteOn()) onPositions.push_back(position + e.offset);
            if (e.message.isNoteOff()) offPositions.push_back(position + e.offset);
        }
    }

    REQUIRE(onPositions.size() == 1);
    REQUIRE(offPositions.size() == 1);
    REQUIRE(onPositions[0] == noteOnSample);
    REQUIRE(std::abs(offPositions[0] - noteOffSample) <= 1);
}

TEST_CASE("ClipScheduler never drops or repeats an event across block boundaries", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);

    // Sixteenth notes for four beats, including ones that land exactly on a
    // block boundary (beat 0) and ones that do not.
    std::vector<MidiNoteData> notes;
    for (int i = 0; i < 16; ++i)
        notes.push_back({60 + (i % 12), i * 0.25, 0.2, 0.7f});
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, notes));

    for (int blockSize : {64, 441, 512, 1024}) {
        // A stopped block resets playback continuity between runs.
        scheduler.processBlock(blockSize, kSampleRate, kBpm, 0, false, false, 0.0, 0.0);
        sink.events.clear();
        for (int64_t position = 0; position < 5 * kSamplesPerBeat; position += blockSize)
            play(scheduler, blockSize, position);

        INFO("block size " << blockSize);
        REQUIRE(sink.count(true) == 16);
        REQUIRE(sink.count(false) == 16);
    }
}

TEST_CASE("ClipScheduler does nothing while the transport is stopped", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));

    scheduler.processBlock(512, kSampleRate, kBpm, 0, false, false, 0.0, 0.0);

    REQUIRE(sink.events.empty());
}

TEST_CASE("ClipScheduler releases held notes when the transport stops", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 4.0, 0.8f}}));

    play(scheduler, 512, 0);
    REQUIRE(sink.count(true) == 1);

    scheduler.processBlock(512, kSampleRate, kBpm, 512, false, false, 0.0, 0.0);
    REQUIRE(sink.countAllNotesOff() == 1);

    // And only once.
    scheduler.processBlock(512, kSampleRate, kBpm, 512, false, false, 0.0, 0.0);
    REQUIRE(sink.countAllNotesOff() == 1);
}

TEST_CASE("ClipScheduler ignores clips outside the block and clips on unknown tracks", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("far", "a", 10.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));
    scheduler.addClip(makeClip("orphan", "no-such-track", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));

    play(scheduler, 512, 0);

    REQUIRE(sink.events.empty());
}

TEST_CASE("ClipScheduler routes each clip to its own track", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sinkA, sinkB;
    scheduler.setTrackSink("a", &sinkA);
    scheduler.setTrackSink("b", &sinkB);
    scheduler.addClip(makeClip("clip-a", "a", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));
    scheduler.addClip(makeClip("clip-b", "b", 0.0, 4.0, {{36, 0.0, 1.0, 0.8f}, {38, 0.0, 1.0, 0.8f}}));

    play(scheduler, 512, 0);

    REQUIRE(sinkA.count(true) == 1);
    REQUIRE(sinkB.count(true) == 2);
    REQUIRE(sinkA.events[0].message.getNoteNumber() == 60);
}

TEST_CASE("ClipScheduler updateClip replaces the clip", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));
    scheduler.updateClip(makeClip("clip-1", "a", 0.0, 4.0, {{72, 0.0, 1.0, 0.8f}}));

    REQUIRE(scheduler.getClipCount() == 1);
    play(scheduler, 512, 0);

    REQUIRE(sink.count(true) == 1);
    REQUIRE(sink.events[0].message.getNoteNumber() == 72);
}

TEST_CASE("ClipScheduler cuts a note at the end of its clip", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    // One-beat clip holding a four-beat note, and a note that starts after the clip ends.
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 1.0, {{60, 0.0, 4.0, 0.8f}, {62, 2.0, 1.0, 0.8f}}));

    std::vector<int64_t> offPositions;
    for (int64_t position = 0; position < 4 * kSamplesPerBeat; position += 512) {
        sink.events.clear();
        play(scheduler, 512, position);
        for (const auto& e : sink.events) {
            REQUIRE(e.message.getNoteNumber() != 62);
            if (e.message.isNoteOff()) offPositions.push_back(position + e.offset);
        }
    }

    REQUIRE(offPositions.size() == 1);
    REQUIRE(offPositions[0] == kSamplesPerBeat);
}

TEST_CASE("ClipScheduler releases a repeated note before striking it again", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    // Two back-to-back notes on the same pitch: the first ends exactly where the second begins.
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 0.01, 0.8f}, {60, 0.01, 0.01, 0.8f}}));

    play(scheduler, 512, 0);

    // on, off, on, off -- never on, on, off, off.
    std::vector<bool> order;
    for (const auto& e : sink.events)
        if (e.message.isNoteOn() || e.message.isNoteOff())
            order.push_back(e.message.isNoteOn());
    REQUIRE(order == std::vector<bool>{true, false, true, false});
}

TEST_CASE("ClipScheduler wraps at the loop end inside a block", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 0.5, 0.8f}}));

    // Loop the first beat. Start 150 samples before the loop end.
    const int64_t start = kSamplesPerBeat - 150;
    play(scheduler, 512, start, true, 0.0, 1.0);

    // The note at beat 0 is struck again where the loop wraps: 150 samples in.
    REQUIRE(sink.count(true) == 1);
    int noteOnOffset = -1;
    int allOffOffset = -1;
    for (const auto& e : sink.events) {
        if (e.message.isNoteOn()) noteOnOffset = e.offset;
        if (e.message.isAllNotesOff()) allOffOffset = e.offset;
    }
    REQUIRE(noteOnOffset == 150);
    REQUIRE(allOffOffset == 150);

    // The next block continues from where the wrap left the playhead (sample
    // 362), which must not be mistaken for a seek.
    sink.events.clear();
    play(scheduler, 512, 512 - 150, true, 0.0, 1.0);
    REQUIRE(sink.countAllNotesOff() == 0);
    REQUIRE(sink.count(true) == 0);
}

TEST_CASE("ClipScheduler releases notes when the playhead jumps", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 8.0, {{60, 0.0, 8.0, 0.8f}}));

    play(scheduler, 512, 0);
    REQUIRE(sink.countAllNotesOff() == 0);

    play(scheduler, 512, 4 * kSamplesPerBeat);  // not 512: a seek
    REQUIRE(sink.countAllNotesOff() == 1);
}

TEST_CASE("ClipScheduler releases notes when the arrangement changes during playback", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 8.0, {{60, 0.0, 8.0, 0.8f}}));

    play(scheduler, 512, 0);
    REQUIRE(sink.count(true) == 1);

    // Deleting the clip also deletes the note-off that would have ended the note.
    scheduler.removeClip("clip-1");
    play(scheduler, 512, 512);

    REQUIRE(sink.countAllNotesOff() == 1);
}

TEST_CASE("ClipScheduler stops sending to a track once its sink is removed", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    RecordingSink sink;
    scheduler.setTrackSink("a", &sink);
    scheduler.addClip(makeClip("clip-1", "a", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));

    scheduler.removeTrackSink("a");
    play(scheduler, 512, 0);

    REQUIRE(sink.events.empty());
}

TEST_CASE("ClipScheduler drives a real instrument", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    PolySynthProcessor synth;
    synth.prepareToPlay(kSampleRate, 512);
    scheduler.setTrackSink("lead", &synth);
    scheduler.addClip(makeClip("clip-1", "lead", 0.0, 4.0, {{60, 0.0, 1.0, 0.8f}}));

    play(scheduler, 512, 0);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE(hasNonZeroSamples(buffer, 0));
}
