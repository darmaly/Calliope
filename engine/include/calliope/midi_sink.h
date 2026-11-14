#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace calliope {

// Anything that can receive MIDI from outside the audio graph (the clip
// scheduler on the audio thread, or note previews from the bridge).
// Implementations must be safe to call from any thread.
class MidiSink {
public:
    virtual ~MidiSink() = default;

    // sampleOffset is relative to the start of the next block to be rendered.
    virtual void addMidiEvent(const juce::MidiMessage& msg, int sampleOffset = 0) = 0;
};

} // namespace calliope
