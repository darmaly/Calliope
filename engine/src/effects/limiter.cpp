#include "calliope/effects/limiter.h"

namespace calliope {

LimiterProcessor::LimiterProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
}

const juce::String LimiterProcessor::getName() const { return "Limiter"; }

void LimiterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    limiter_.prepare(spec);
    limiter_.reset();
}

void LimiterProcessor::releaseResources() {}

void LimiterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    if (bypassed.load(std::memory_order_relaxed))
        return;

    limiter_.setThreshold(threshold.load(std::memory_order_relaxed));
    limiter_.setRelease(release.load(std::memory_order_relaxed));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    limiter_.process(context);
}

double LimiterProcessor::getTailLengthSeconds() const { return 0.0; }
bool LimiterProcessor::acceptsMidi() const { return false; }
bool LimiterProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* LimiterProcessor::createEditor() { return nullptr; }
bool LimiterProcessor::hasEditor() const { return false; }
int LimiterProcessor::getNumPrograms() { return 1; }
int LimiterProcessor::getCurrentProgram() { return 0; }
void LimiterProcessor::setCurrentProgram(int) {}
const juce::String LimiterProcessor::getProgramName(int) { return {}; }
void LimiterProcessor::changeProgramName(int, const juce::String&) {}
void LimiterProcessor::getStateInformation(juce::MemoryBlock&) {}
void LimiterProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
