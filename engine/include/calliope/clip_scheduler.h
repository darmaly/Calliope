#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "calliope/midi_sink.h"
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace calliope {

struct MidiNoteData {
    int pitch;           // 0-127
    double startBeat;    // relative to clip start
    double lengthBeats;
    float velocity;      // 0.0-1.0
};

struct MidiClipData {
    std::string clipId;
    std::string trackId;
    double startBeat;         // absolute beat position on timeline
    double lengthBeats;
    std::vector<MidiNoteData> notes;
};

// Turns clips into MIDI for each track's instrument.
//
// Edits (clips, track sinks) happen on a control thread and publish an
// immutable snapshot. The audio thread reads the latest snapshot without
// taking the edit lock, and positions every event on its exact sample.
class ClipScheduler {
public:
    ClipScheduler();
    ~ClipScheduler();

    // --- Control thread ---
    void addClip(const MidiClipData& clip);
    void removeClip(const std::string& clipId);
    void updateClip(const MidiClipData& clip);
    void clearClips();
    void removeClipsForTrack(const std::string& trackId);
    int getClipCount() const;
    std::vector<MidiClipData> getClips() const;

    // Beat at which the last clip ends (0 when there are no clips).
    double getEndBeat() const;

    // Register where a track's MIDI goes. keepAlive is held for as long as any
    // snapshot can still reach the sink, so the sink may be owned elsewhere.
    void setTrackSink(const std::string& trackId, MidiSink* sink,
                      std::shared_ptr<void> keepAlive = {});
    void removeTrackSink(const std::string& trackId);
    void clearTrackSinks();

    // --- Audio thread ---
    // Call once per block, before the instruments render it.
    // blockStartSample is the transport position of the block's first sample.
    void processBlock(int numSamples,
                      double sampleRate,
                      double bpm,
                      int64_t blockStartSample,
                      bool isPlaying,
                      bool isLooping,
                      double loopStartBeat,
                      double loopEndBeat);

private:
    static constexpr double kTicksPerBeat = 1000000.0;

    struct Event {
        int64_t tick;      // absolute position in 1/1,000,000 beat
        int sinkIndex;
        int pitch;
        float velocity;
        bool noteOn;
    };

    struct Snapshot {
        std::vector<Event> events;                    // sorted by (tick, offs first)
        std::vector<MidiSink*> sinks;
        std::vector<std::shared_ptr<void>> keepAlive;
    };

    struct SinkEntry {
        MidiSink* sink = nullptr;
        std::shared_ptr<void> keepAlive;
    };

    // Control-thread state
    mutable std::mutex editMutex_;
    std::map<std::string, MidiClipData> clips_;
    std::map<std::string, SinkEntry> sinks_;
    std::vector<std::shared_ptr<const Snapshot>> retired_;

    // Shared: written with std::atomic_store, read with std::atomic_load
    std::shared_ptr<const Snapshot> current_;

    // Audio-thread state
    std::shared_ptr<const Snapshot> active_;
    bool wasPlaying_ = false;
    int64_t expectedNextSample_ = -1;

    void publish();  // editMutex_ must be held

    static void allNotesOff(const Snapshot& snapshot, int sampleOffset);
    static void emitRange(const Snapshot& snapshot, double samplesPerBeat,
                          int64_t rangeStart, int64_t rangeEnd, int bufferOffset);
};

} // namespace calliope
