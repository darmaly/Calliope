#include "calliope/effects/delay.h"
#include <algorithm>

namespace calliope {

DelayProcessor::DelayProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo())),
      delayLines_{
          juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>(576000),
          juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>(576000)
      }
{
}

const juce::String DelayProcessor::getName() const { return "Delay"; }

void DelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1; // Each delay line processes one channel

    for (auto& dl : delayLines_) {
        dl.setMaximumDelayInSamples(576000); // 12s at 48kHz
        dl.prepare(spec);
        dl.reset();
    }

    juce::dsp::ProcessSpec stereoSpec;
    stereoSpec.sampleRate = sampleRate;
    stereoSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    stereoSpec.numChannels = 2;

    dryWetMixer_.prepare(stereoSpec);
    dryWetMixer_.reset();
}

void DelayProcessor::releaseResources() {}

void DelayProcessor::setCurrentBpm(float bpm)
{
    currentBpm_.store(bpm, std::memory_order_relaxed);
}

float DelayProcessor::getDelayTimeInSamples() const
{
    float bpm = currentBpm_.load(std::memory_order_relaxed);
    float noteValue = syncNoteValue.load(std::memory_order_relaxed);

    if (bpm <= 0.0f) bpm = 120.0f;

    float beatsPerSecond = bpm / 60.0f;
    float delaySeconds = noteValue / beatsPerSecond;

    return delaySeconds * static_cast<float>(sampleRate_);
}

void DelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    if (bypassed.load(std::memory_order_relaxed))
        return;

    float delaySamples = getDelayTimeInSamples();
    float fb = std::clamp(feedback.load(std::memory_order_relaxed), 0.0f, 0.98f);
    bool pingPong = pingPongEnabled.load(std::memory_order_relaxed);
    float mix = wetDry.load(std::memory_order_relaxed);

    dryWetMixer_.setWetMixProportion(mix);
    dryWetMixer_.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    int numChannels = std::min(buffer.getNumChannels(), 2);
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        for (int ch = 0; ch < numChannels; ++ch) {
            float input = buffer.getSample(ch, i);
            float delayed = delayLines_[ch].popSample(0, delaySamples);

            float feedbackSample = delayed * fb;
            if (pingPong && numChannels == 2) {
                int otherCh = 1 - ch;
                delayLines_[otherCh].pushSample(0, input + feedbackSample);
            } else {
                delayLines_[ch].pushSample(0, input + feedbackSample);
            }

            buffer.setSample(ch, i, delayed);
        }
    }

    dryWetMixer_.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}

double DelayProcessor::getTailLengthSeconds() const { return 12.0; }
bool DelayProcessor::acceptsMidi() const { return false; }
bool DelayProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* DelayProcessor::createEditor() { return nullptr; }
bool DelayProcessor::hasEditor() const { return false; }
int DelayProcessor::getNumPrograms() { return 1; }
int DelayProcessor::getCurrentProgram() { return 0; }
void DelayProcessor::setCurrentProgram(int) {}
const juce::String DelayProcessor::getProgramName(int) { return {}; }
void DelayProcessor::changeProgramName(int, const juce::String&) {}
void DelayProcessor::getStateInformation(juce::MemoryBlock&) {}
void DelayProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
