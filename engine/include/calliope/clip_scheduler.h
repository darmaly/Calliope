#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace calliope {

// Forward declarations
class PolySynthProcessor;
class BassSynthProcessor;
class DrumMachineProcessor;

struct MidiNoteData {
    int pitch;           // 0-127
    double startBeat;    // relative to clip start
    double lengthBeats;
    float velocity;      // 0.0-1.0
};

struct MidiClipData {
    std::string clipId;
    std::string trackId;      // "polysynth", "basssynth", "drumMachine"
    double startBeat;         // absolute beat position on timeline
    double lengthBeats;
    std::vector<MidiNoteData> notes;
};

class ClipScheduler {
public:
    ClipScheduler();

    // Clip CRUD (called from bridge thread, mutex-protected)
    void addClip(const MidiClipData& clip);
    void removeClip(const std::string& clipId);
    void updateClip(const MidiClipData& clip);
    void clearClips();
    int getClipCount() const;

    // Called each audio callback BEFORE instrument processBlock.
    // Reads transport position, determines which MIDI events fall in the current
    // block's beat range, and dispatches noteOn/noteOff via addMidiEvent.
    void processBlock(
        int numSamples,
        double sampleRate,
        double bpm,
        double currentPpqPosition,
        bool isPlaying,
        bool isLooping,
        double loopStartBeat,
        double loopEndBeat,
        PolySynthProcessor* polySynth,
        BassSynthProcessor* bassSynth,
        DrumMachineProcessor* drumMachine
    );

private:
    mutable std::mutex clipMutex_;
    std::map<std::string, MidiClipData> clips_;

    // Track active notes to send noteOff when stopping
    struct ActiveNote {
        std::string trackId;
        int pitch;
        double endBeat;  // absolute beat where noteOff should fire
    };
    std::vector<ActiveNote> activeNotes_;

    void dispatchMidiEvent(
        const std::string& trackId, const juce::MidiMessage& msg,
        PolySynthProcessor* polySynth,
        BassSynthProcessor* bassSynth,
        DrumMachineProcessor* drumMachine
    );

    void allNotesOff(
        PolySynthProcessor* polySynth,
        BassSynthProcessor* bassSynth,
        DrumMachineProcessor* drumMachine
    );

    bool wasPlaying_ = false;
};

} // namespace calliope
