#include "calliope/master_bus.h"
#include <cmath>

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

    // Meter computation -- after volume application
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < std::min(buffer.getNumChannels(), 2); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        float sum = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            float s = data[i];
            sum += s * s;
            float absVal = std::fabs(s);
            if (absVal > peak) peak = absVal;
        }
        float rms = std::sqrt(sum / static_cast<float>(numSamples));
        if (ch == 0) {
            meterData_.rmsLeft.store(rms, std::memory_order_relaxed);
            meterData_.peakLeft.store(peak, std::memory_order_relaxed);
        } else {
            meterData_.rmsRight.store(rms, std::memory_order_relaxed);
            meterData_.peakRight.store(peak, std::memory_order_relaxed);
        }
    }
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
