#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/poly_synth_voice.h"
#include "calliope/instruments/poly_synth_sound.h"

namespace calliope {

PolySynthProcessor::PolySynthProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    for (int i = 0; i < 16; ++i)
        synth_.addVoice(new PolySynthVoice(*this));

    synth_.addSound(new PolySynthSound());
}

const juce::String PolySynthProcessor::getName() const { return "PolySynth"; }

void PolySynthProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synth_.setCurrentPlaybackSampleRate(sampleRate);
}

void PolySynthProcessor::releaseResources() {}

void PolySynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    {
        const juce::SpinLock::ScopedLockType lock(midiLock_);
        midi.addEvents(pendingMidi_, 0, -1, 0);
        pendingMidi_.clear();
    }

    buffer.clear();
    synth_.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    float gain = masterGain.load(std::memory_order_relaxed);
    buffer.applyGain(gain);
}

void PolySynthProcessor::addMidiEvent(const juce::MidiMessage& msg)
{
    const juce::SpinLock::ScopedLockType lock(midiLock_);
    pendingMidi_.addEvent(msg, 0);
}

double PolySynthProcessor::getTailLengthSeconds() const { return 0.0; }
bool PolySynthProcessor::acceptsMidi() const { return true; }
bool PolySynthProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* PolySynthProcessor::createEditor() { return nullptr; }
bool PolySynthProcessor::hasEditor() const { return false; }
int PolySynthProcessor::getNumPrograms() { return 1; }
int PolySynthProcessor::getCurrentProgram() { return 0; }
void PolySynthProcessor::setCurrentProgram(int) {}
const juce::String PolySynthProcessor::getProgramName(int) { return {}; }
void PolySynthProcessor::changeProgramName(int, const juce::String&) {}
void PolySynthProcessor::getStateInformation(juce::MemoryBlock&) {}
void PolySynthProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
