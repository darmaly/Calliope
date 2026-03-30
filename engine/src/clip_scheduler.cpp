#include "calliope/clip_scheduler.h"
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/drum_machine.h"
#include <algorithm>

namespace calliope {

ClipScheduler::ClipScheduler() = default;

void ClipScheduler::addClip(const MidiClipData& clip)
{
    std::lock_guard<std::mutex> lock(clipMutex_);
    clips_[clip.clipId] = clip;
}

void ClipScheduler::removeClip(const std::string& clipId)
{
    std::lock_guard<std::mutex> lock(clipMutex_);
    clips_.erase(clipId);
}

void ClipScheduler::updateClip(const MidiClipData& clip)
{
    std::lock_guard<std::mutex> lock(clipMutex_);
    clips_[clip.clipId] = clip;
}

void ClipScheduler::clearClips()
{
    std::lock_guard<std::mutex> lock(clipMutex_);
    clips_.clear();
    activeNotes_.clear();
}

int ClipScheduler::getClipCount() const
{
    std::lock_guard<std::mutex> lock(clipMutex_);
    return static_cast<int>(clips_.size());
}

void ClipScheduler::processBlock(
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
    DrumMachineProcessor* drumMachine)
{
    // If not playing: send allNotesOff if we were previously playing
    if (!isPlaying) {
        if (wasPlaying_) {
            allNotesOff(polySynth, bassSynth, drumMachine);
            wasPlaying_ = false;
        }
        return;
    }

    wasPlaying_ = true;

    // Calculate beat range for this block
    double beatsPerSecond = bpm / 60.0;
    double blockDurationSeconds = static_cast<double>(numSamples) / sampleRate;
    double blockDurationBeats = blockDurationSeconds * beatsPerSecond;

    double blockStartBeat = currentPpqPosition;
    double blockEndBeat = currentPpqPosition + blockDurationBeats;

    std::lock_guard<std::mutex> lock(clipMutex_);

    // Iterate all clips and check for note events in the current block range
    for (const auto& [clipId, clip] : clips_) {
        for (const auto& note : clip.notes) {
            // Calculate absolute beat positions for this note
            double noteStartAbsolute = clip.startBeat + note.startBeat;
            double noteEndAbsolute = noteStartAbsolute + note.lengthBeats;

            // Check if noteOn falls in [blockStartBeat, blockEndBeat)
            if (noteStartAbsolute >= blockStartBeat && noteStartAbsolute < blockEndBeat) {
                auto noteOnMsg = juce::MidiMessage::noteOn(1, note.pitch, note.velocity);
                dispatchMidiEvent(clip.trackId, noteOnMsg, polySynth, bassSynth, drumMachine);

                // Track active note for noteOff
                activeNotes_.push_back({clip.trackId, note.pitch, noteEndAbsolute});
            }

            // Check if noteOff falls in [blockStartBeat, blockEndBeat)
            if (noteEndAbsolute >= blockStartBeat && noteEndAbsolute < blockEndBeat) {
                auto noteOffMsg = juce::MidiMessage::noteOff(1, note.pitch);
                dispatchMidiEvent(clip.trackId, noteOffMsg, polySynth, bassSynth, drumMachine);
            }
        }
    }

    // Also process active notes whose end beat falls in range
    // (handles notes that were started in a previous block)
    activeNotes_.erase(
        std::remove_if(activeNotes_.begin(), activeNotes_.end(),
            [&](const ActiveNote& an) {
                if (an.endBeat >= blockStartBeat && an.endBeat < blockEndBeat) {
                    // noteOff already dispatched above via the clip iteration
                    return true; // remove from active
                }
                if (an.endBeat < blockStartBeat) {
                    // Missed noteOff (e.g., due to position jump) -- send it now
                    auto noteOffMsg = juce::MidiMessage::noteOff(1, an.pitch);
                    dispatchMidiEvent(an.trackId, noteOffMsg, polySynth, bassSynth, drumMachine);
                    return true;
                }
                return false;
            }),
        activeNotes_.end());
}

void ClipScheduler::dispatchMidiEvent(
    const std::string& trackId, const juce::MidiMessage& msg,
    PolySynthProcessor* polySynth,
    BassSynthProcessor* bassSynth,
    DrumMachineProcessor* drumMachine)
{
    if (trackId == "polysynth" && polySynth) {
        polySynth->addMidiEvent(msg);
    } else if (trackId == "basssynth" && bassSynth) {
        bassSynth->addMidiEvent(msg);
    } else if (trackId == "drumMachine" && drumMachine) {
        drumMachine->addMidiEvent(msg);
    }
}

void ClipScheduler::allNotesOff(
    PolySynthProcessor* polySynth,
    BassSynthProcessor* bassSynth,
    DrumMachineProcessor* drumMachine)
{
    // Send CC 123 (All Notes Off) to all instruments
    auto allOff = juce::MidiMessage::allNotesOff(1);

    if (polySynth)   polySynth->addMidiEvent(allOff);
    if (bassSynth)   bassSynth->addMidiEvent(allOff);
    if (drumMachine) drumMachine->addMidiEvent(allOff);

    activeNotes_.clear();
}

} // namespace calliope
