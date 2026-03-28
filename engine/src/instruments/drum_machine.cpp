#include "calliope/instruments/drum_machine.h"

namespace calliope {

DrumMachineProcessor::DrumMachineProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    formatManager_.registerBasicFormats();

    // Add 16 SamplerVoice instances (one per pad for simultaneous playback)
    for (int i = 0; i < kNumPads; ++i)
        synth_.addVoice(new juce::SamplerVoice());
}

const juce::String DrumMachineProcessor::getName() const { return "DrumMachine"; }

void DrumMachineProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synth_.setCurrentPlaybackSampleRate(sampleRate);
}

void DrumMachineProcessor::releaseResources() {}

void DrumMachineProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    // Merge pending MIDI events
    juce::MidiBuffer incomingMidi;
    {
        const juce::SpinLock::ScopedLockType lock(midiLock_);
        incomingMidi.swapWith(pendingMidi_);
    }

    buffer.clear();
    synth_.renderNextBlock(buffer, incomingMidi, 0, buffer.getNumSamples());

    // Apply volume
    float vol = volume.load(std::memory_order_relaxed);
    if (vol != 1.0f)
        buffer.applyGain(vol);
}

bool DrumMachineProcessor::loadSample(int padIndex, const juce::File& file)
{
    if (padIndex < 0 || padIndex >= kNumPads)
        return false;

    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));
    if (!reader)
        return false;

    int midiNote = kFirstMidiNote + padIndex;

    // Remove any existing SamplerSound for this note
    for (int i = synth_.getNumSounds() - 1; i >= 0; --i) {
        if (auto* existingSound = dynamic_cast<juce::SamplerSound*>(synth_.getSound(i).get())) {
            if (existingSound->appliesToNote(midiNote)) {
                synth_.removeSound(i);
                break;
            }
        }
    }

    // Create note range for this specific MIDI note
    juce::BigInteger noteRange;
    noteRange.setBit(midiNote);

    // Add new SamplerSound
    synth_.addSound(new juce::SamplerSound(
        file.getFileNameWithoutExtension(),
        *reader,
        noteRange,
        midiNote,    // root MIDI note
        0.001,       // attack time (seconds)
        0.01,        // release time (seconds)
        30.0         // max sample length (seconds)
    ));

    padNames_[static_cast<size_t>(padIndex)] = file.getFileNameWithoutExtension();
    return true;
}

void DrumMachineProcessor::clearSample(int padIndex)
{
    if (padIndex < 0 || padIndex >= kNumPads)
        return;

    int midiNote = kFirstMidiNote + padIndex;

    for (int i = synth_.getNumSounds() - 1; i >= 0; --i) {
        if (auto* existingSound = dynamic_cast<juce::SamplerSound*>(synth_.getSound(i).get())) {
            if (existingSound->appliesToNote(midiNote)) {
                synth_.removeSound(i);
                break;
            }
        }
    }

    padNames_[static_cast<size_t>(padIndex)] = juce::String();
}

juce::String DrumMachineProcessor::getSampleName(int padIndex) const
{
    if (padIndex < 0 || padIndex >= kNumPads)
        return {};
    return padNames_[static_cast<size_t>(padIndex)];
}

std::vector<juce::String> DrumMachineProcessor::getSampleNames() const
{
    return std::vector<juce::String>(padNames_.begin(), padNames_.end());
}

void DrumMachineProcessor::addMidiEvent(const juce::MidiMessage& msg)
{
    const juce::SpinLock::ScopedLockType lock(midiLock_);
    pendingMidi_.addEvent(msg, 0);
}

double DrumMachineProcessor::getTailLengthSeconds() const { return 0.0; }
bool DrumMachineProcessor::acceptsMidi() const { return true; }
bool DrumMachineProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* DrumMachineProcessor::createEditor() { return nullptr; }
bool DrumMachineProcessor::hasEditor() const { return false; }
int DrumMachineProcessor::getNumPrograms() { return 1; }
int DrumMachineProcessor::getCurrentProgram() { return 0; }
void DrumMachineProcessor::setCurrentProgram(int) {}
const juce::String DrumMachineProcessor::getProgramName(int) { return {}; }
void DrumMachineProcessor::changeProgramName(int, const juce::String&) {}
void DrumMachineProcessor::getStateInformation(juce::MemoryBlock&) {}
void DrumMachineProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
