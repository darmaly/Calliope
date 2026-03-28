#include "calliope/effects/reverb.h"

namespace calliope {

ReverbProcessor::ReverbProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
}

const juce::String ReverbProcessor::getName() const { return "Reverb"; }

void ReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    reverb_.prepare(spec);
    reverb_.reset();

    preDelayLine_.setMaximumDelayInSamples(static_cast<int>(sampleRate)); // 1 second max
    preDelayLine_.prepare(spec);
    preDelayLine_.reset();
}

void ReverbProcessor::releaseResources() {}

void ReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    if (bypassed.load(std::memory_order_relaxed))
        return;

    // Update reverb parameters
    juce::Reverb::Parameters params;
    params.roomSize = roomSize.load(std::memory_order_relaxed);
    params.damping = damping.load(std::memory_order_relaxed);
    params.wetLevel = wetLevel.load(std::memory_order_relaxed);
    params.dryLevel = dryLevel.load(std::memory_order_relaxed);
    params.width = 1.0f;
    params.freezeMode = 0.0f;
    reverb_.setParameters(params);

    // Pre-delay processing
    float preDelayMs = preDelay.load(std::memory_order_relaxed);
    if (preDelayMs > 0.0f) {
        float delaySamples = (preDelayMs / 1000.0f) * static_cast<float>(sampleRate_);
        preDelayLine_.setDelay(delaySamples);

        for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                preDelayLine_.pushSample(ch, data[i]);
                data[i] = preDelayLine_.popSample(ch);
            }
        }
    }

    // Process reverb
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb_.process(context);
}

double ReverbProcessor::getTailLengthSeconds() const { return 2.0; }
bool ReverbProcessor::acceptsMidi() const { return false; }
bool ReverbProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* ReverbProcessor::createEditor() { return nullptr; }
bool ReverbProcessor::hasEditor() const { return false; }
int ReverbProcessor::getNumPrograms() { return 1; }
int ReverbProcessor::getCurrentProgram() { return 0; }
void ReverbProcessor::setCurrentProgram(int) {}
const juce::String ReverbProcessor::getProgramName(int) { return {}; }
void ReverbProcessor::changeProgramName(int, const juce::String&) {}
void ReverbProcessor::getStateInformation(juce::MemoryBlock&) {}
void ReverbProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
