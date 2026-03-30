#include <catch2/catch_test_macros.hpp>
#include "calliope/clip_scheduler.h"
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/drum_machine.h"
#include <cmath>

using namespace calliope;

namespace {

bool hasNonZeroSamples(const juce::AudioBuffer<float>& buffer, int channel = 0)
{
    auto* data = buffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        if (data[i] != 0.0f) return true;
    return false;
}

} // namespace

TEST_CASE("ClipScheduler addClip stores a clip and removeClip removes it", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    REQUIRE(scheduler.getClipCount() == 0);

    MidiClipData clip;
    clip.clipId = "clip-1";
    clip.trackId = "polysynth";
    clip.startBeat = 0.0;
    clip.lengthBeats = 4.0;
    clip.notes.push_back({60, 0.0, 1.0, 0.8f});

    scheduler.addClip(clip);
    REQUIRE(scheduler.getClipCount() == 1);

    scheduler.removeClip("clip-1");
    REQUIRE(scheduler.getClipCount() == 0);
}

TEST_CASE("ClipScheduler clearClips removes all clips", "[clip_scheduler]")
{
    ClipScheduler scheduler;

    MidiClipData clip1;
    clip1.clipId = "clip-1";
    clip1.trackId = "polysynth";
    clip1.startBeat = 0.0;
    clip1.lengthBeats = 4.0;

    MidiClipData clip2;
    clip2.clipId = "clip-2";
    clip2.trackId = "basssynth";
    clip2.startBeat = 4.0;
    clip2.lengthBeats = 4.0;

    scheduler.addClip(clip1);
    scheduler.addClip(clip2);
    REQUIRE(scheduler.getClipCount() == 2);

    scheduler.clearClips();
    REQUIRE(scheduler.getClipCount() == 0);
}

TEST_CASE("ClipScheduler processBlock dispatches noteOn at correct beat position", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    PolySynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    // Create a clip at beat 0 with a note at relative beat 0
    MidiClipData clip;
    clip.clipId = "clip-1";
    clip.trackId = "polysynth";
    clip.startBeat = 0.0;
    clip.lengthBeats = 4.0;
    clip.notes.push_back({60, 0.0, 1.0, 0.8f}); // pitch=60, start=0, length=1 beat, vel=0.8

    scheduler.addClip(clip);

    // Process at beat 0 with BPM=120, sampleRate=44100, numSamples=512
    // Beat range: 0.0 to 0.0 + (512 / 44100) * (120 / 60) = 0.0 + 0.02322... ~= 0.0232 beats
    scheduler.processBlock(
        512, 44100.0, 120.0,
        0.0,   // currentPpqPosition (beat 0)
        true,  // isPlaying
        false, // isLooping
        0.0, 0.0,
        &synth, nullptr, nullptr
    );

    // Now process the synth -- the noteOn should have been dispatched via addMidiEvent
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE(hasNonZeroSamples(buffer, 0));
}

TEST_CASE("ClipScheduler processBlock does nothing when transport is stopped", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    PolySynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    MidiClipData clip;
    clip.clipId = "clip-1";
    clip.trackId = "polysynth";
    clip.startBeat = 0.0;
    clip.lengthBeats = 4.0;
    clip.notes.push_back({60, 0.0, 1.0, 0.8f});

    scheduler.addClip(clip);

    // Process with isPlaying=false
    scheduler.processBlock(
        512, 44100.0, 120.0,
        0.0,   // at beat 0 where note is
        false, // NOT playing
        false, 0.0, 0.0,
        &synth, nullptr, nullptr
    );

    // Synth should produce silence since no MIDI was dispatched
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE_FALSE(hasNonZeroSamples(buffer, 0));
}

TEST_CASE("ClipScheduler processBlock with clip outside current block range dispatches no events", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    PolySynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    // Clip starts at beat 10
    MidiClipData clip;
    clip.clipId = "clip-1";
    clip.trackId = "polysynth";
    clip.startBeat = 10.0;
    clip.lengthBeats = 4.0;
    clip.notes.push_back({60, 0.0, 1.0, 0.8f});

    scheduler.addClip(clip);

    // Process at beat 0 -- clip starts at beat 10, way outside range
    scheduler.processBlock(
        512, 44100.0, 120.0,
        0.0,   // at beat 0
        true,  // playing
        false, 0.0, 0.0,
        &synth, nullptr, nullptr
    );

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE_FALSE(hasNonZeroSamples(buffer, 0));
}

TEST_CASE("ClipScheduler updateClip replaces clip data", "[clip_scheduler]")
{
    ClipScheduler scheduler;
    PolySynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    // Add clip with note at pitch 60
    MidiClipData clip;
    clip.clipId = "clip-1";
    clip.trackId = "polysynth";
    clip.startBeat = 0.0;
    clip.lengthBeats = 4.0;
    clip.notes.push_back({60, 0.0, 1.0, 0.8f});
    scheduler.addClip(clip);

    // Update: move clip to beat 10 (out of range for beat 0)
    MidiClipData updated;
    updated.clipId = "clip-1";
    updated.trackId = "polysynth";
    updated.startBeat = 10.0;
    updated.lengthBeats = 4.0;
    updated.notes.push_back({72, 0.0, 1.0, 0.8f});
    scheduler.updateClip(updated);

    REQUIRE(scheduler.getClipCount() == 1);

    // Process at beat 0 -- clip is now at beat 10, so no events
    scheduler.processBlock(
        512, 44100.0, 120.0,
        0.0, true, false, 0.0, 0.0,
        &synth, nullptr, nullptr
    );

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE_FALSE(hasNonZeroSamples(buffer, 0));
}
