#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/bass_synth_voice.h"
#include "calliope/instruments/bass_synth_sound.h"

namespace calliope {

BassSynthProcessor::BassSynthProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    // 4 voices: low polyphony for bass, saves CPU
    for (int i = 0; i < 4; ++i)
        synth_.addVoice(new BassSynthVoice(*this));

    synth_.addSound(new BassSynthSound());
}

const juce::String BassSynthProcessor::getName() const { return "BassSynth"; }

void BassSynthProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synth_.setCurrentPlaybackSampleRate(sampleRate);
}

void BassSynthProcessor::releaseResources() {}

void BassSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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

void BassSynthProcessor::addMidiEvent(const juce::MidiMessage& msg)
{
    const juce::SpinLock::ScopedLockType lock(midiLock_);
    pendingMidi_.addEvent(msg, 0);
}

double BassSynthProcessor::getTailLengthSeconds() const { return 0.0; }
bool BassSynthProcessor::acceptsMidi() const { return true; }
bool BassSynthProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* BassSynthProcessor::createEditor() { return nullptr; }
bool BassSynthProcessor::hasEditor() const { return false; }
int BassSynthProcessor::getNumPrograms() { return 1; }
int BassSynthProcessor::getCurrentProgram() { return 0; }
void BassSynthProcessor::setCurrentProgram(int) {}
const juce::String BassSynthProcessor::getProgramName(int) { return {}; }
void BassSynthProcessor::changeProgramName(int, const juce::String&) {}
void BassSynthProcessor::getStateInformation(juce::MemoryBlock&) {}
void BassSynthProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
