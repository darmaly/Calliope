#include "calliope/master_bus.h"

namespace calliope {

MasterBusProcessor::MasterBusProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
}

const juce::String MasterBusProcessor::getName() const { return "MasterBus"; }

void MasterBusProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // No-op for now — future: prepare insert chain processors
}

void MasterBusProcessor::releaseResources() {}

void MasterBusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    buffer.applyGain(masterVolume.load(std::memory_order_relaxed));
}

double MasterBusProcessor::getTailLengthSeconds() const { return 0.0; }
bool MasterBusProcessor::acceptsMidi() const { return false; }
bool MasterBusProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* MasterBusProcessor::createEditor() { return nullptr; }
bool MasterBusProcessor::hasEditor() const { return false; }
int MasterBusProcessor::getNumPrograms() { return 1; }
int MasterBusProcessor::getCurrentProgram() { return 0; }
void MasterBusProcessor::setCurrentProgram(int) {}
const juce::String MasterBusProcessor::getProgramName(int) { return {}; }
void MasterBusProcessor::changeProgramName(int, const juce::String&) {}
void MasterBusProcessor::getStateInformation(juce::MemoryBlock&) {}
void MasterBusProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
