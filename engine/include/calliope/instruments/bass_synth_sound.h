#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace calliope {

class BassSynthSound : public juce::SynthesiserSound {
public:
    BassSynthSound() = default;

    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

} // namespace calliope
