#include "calliope/insert_chain_processor.h"

namespace calliope {

InsertChainProcessor::InsertChainProcessor(const juce::String& trackId)
    : trackId_(trackId)
{
}

const juce::String InsertChainProcessor::getName() const
{
    return "InsertChain:" + trackId_;
}

void InsertChainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    chain_.prepareToPlay(sampleRate, samplesPerBlock);
}

void InsertChainProcessor::releaseResources()
{
}

void InsertChainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    chain_.processBlock(buffer, midi);
}

double InsertChainProcessor::getTailLengthSeconds() const
{
    return 12.0;
}

bool InsertChainProcessor::acceptsMidi() const { return false; }
bool InsertChainProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* InsertChainProcessor::createEditor() { return nullptr; }
bool InsertChainProcessor::hasEditor() const { return false; }
int InsertChainProcessor::getNumPrograms() { return 1; }
int InsertChainProcessor::getCurrentProgram() { return 0; }
void InsertChainProcessor::setCurrentProgram(int) {}
const juce::String InsertChainProcessor::getProgramName(int) { return {}; }
void InsertChainProcessor::changeProgramName(int, const juce::String&) {}
void InsertChainProcessor::getStateInformation(juce::MemoryBlock&) {}
void InsertChainProcessor::setStateInformation(const void*, int) {}

InsertChain& InsertChainProcessor::getInsertChain()
{
    return chain_;
}

const InsertChain& InsertChainProcessor::getInsertChain() const
{
    return chain_;
}

const juce::String& InsertChainProcessor::getTrackId() const
{
    return trackId_;
}

} // namespace calliope
