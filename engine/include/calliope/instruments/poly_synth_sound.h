#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace calliope {

class PolySynthSound : public juce::SynthesiserSound {
public:
    PolySynthSound() = default;

    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

} // namespace calliope
