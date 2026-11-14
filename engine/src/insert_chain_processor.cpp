#include "calliope/insert_chain_processor.h"
#include <cmath>

namespace calliope {

InsertChainProcessor::InsertChainProcessor(const juce::String& trackId)
    // The buses must be declared and enabled: AudioProcessor's default
    // constructor leaves them disabled, which gives the node zero channels and
    // makes the graph refuse every connection to or from it.
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo())),
      trackId_(trackId)
{
}

const juce::String InsertChainProcessor::getName() const
{
    return "InsertChain:" + trackId_;
}

void InsertChainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    chain_.prepareToPlay(sampleRate, samplesPerBlock);
    gainLeft_.reset(sampleRate, 0.02);
    gainRight_.reset(sampleRate, 0.02);
}

void InsertChainProcessor::releaseResources()
{
}

void InsertChainProcessor::reset()
{
    chain_.reset();
    // Jump straight to the current fader position instead of ramping to it.
    updateGainTargets();
    gainLeft_.setCurrentAndTargetValue(gainLeft_.getTargetValue());
    gainRight_.setCurrentAndTargetValue(gainRight_.getTargetValue());
}

void InsertChainProcessor::updateGainTargets()
{
    // Constant-power pan law, normalised so that centre is unity on both sides.
    const bool silent = mute.load(std::memory_order_relaxed)
                     || soloMuted.load(std::memory_order_relaxed);
    const float gain = silent ? 0.0f : juce::jlimit(0.0f, 2.0f, volume.load(std::memory_order_relaxed));
    const float panValue = juce::jlimit(-1.0f, 1.0f, pan.load(std::memory_order_relaxed));
    const float angle = (panValue + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    const float sqrt2 = juce::MathConstants<float>::sqrt2;

    gainLeft_.setTargetValue(gain * std::cos(angle) * sqrt2);
    gainRight_.setTargetValue(gain * std::sin(angle) * sqrt2);
}

void InsertChainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    chain_.processBlock(buffer, midi);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), 2);

    // Fader, pan and mute
    updateGainTargets();

    if (numChannels > 0)
        gainLeft_.applyGain(buffer.getWritePointer(0), numSamples);
    if (numChannels > 1)
        gainRight_.applyGain(buffer.getWritePointer(1), numSamples);
    else
        gainRight_.skip(numSamples);

    // Post-fader metering
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        float sum = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            const float s = data[i];
            sum += s * s;
            peak = std::max(peak, std::fabs(s));
        }
        const float rms = numSamples > 0 ? std::sqrt(sum / static_cast<float>(numSamples)) : 0.0f;
        if (ch == 0) {
            meterData_.rmsLeft.store(rms, std::memory_order_relaxed);
            meterData_.peakLeft.store(peak, std::memory_order_relaxed);
        } else {
            meterData_.rmsRight.store(rms, std::memory_order_relaxed);
            meterData_.peakRight.store(peak, std::memory_order_relaxed);
        }
    }
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
