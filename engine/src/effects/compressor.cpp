#include "calliope/effects/compressor.h"

namespace calliope {

CompressorProcessor::CompressorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
}

const juce::String CompressorProcessor::getName() const { return "Compressor"; }

void CompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    compressor_.prepare(spec);
    compressor_.reset();
}

void CompressorProcessor::releaseResources() {}

void CompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    if (bypassed.load(std::memory_order_relaxed))
        return;

    compressor_.setThreshold(threshold.load(std::memory_order_relaxed));
    compressor_.setRatio(ratio.load(std::memory_order_relaxed));
    compressor_.setAttack(attack.load(std::memory_order_relaxed));
    compressor_.setRelease(release.load(std::memory_order_relaxed));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor_.process(context);

    float makeupDb = makeupGain.load(std::memory_order_relaxed);
    if (makeupDb != 0.0f) {
        float gain = juce::Decibels::decibelsToGain(makeupDb);
        buffer.applyGain(gain);
    }
}

double CompressorProcessor::getTailLengthSeconds() const { return 0.0; }
bool CompressorProcessor::acceptsMidi() const { return false; }
bool CompressorProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* CompressorProcessor::createEditor() { return nullptr; }
bool CompressorProcessor::hasEditor() const { return false; }
int CompressorProcessor::getNumPrograms() { return 1; }
int CompressorProcessor::getCurrentProgram() { return 0; }
void CompressorProcessor::setCurrentProgram(int) {}
const juce::String CompressorProcessor::getProgramName(int) { return {}; }
void CompressorProcessor::changeProgramName(int, const juce::String&) {}
void CompressorProcessor::getStateInformation(juce::MemoryBlock&) {}
void CompressorProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
