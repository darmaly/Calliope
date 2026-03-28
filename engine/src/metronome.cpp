#include "calliope/metronome.h"
#include <cmath>

namespace calliope {

MetronomeProcessor::MetronomeProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
}

const juce::String MetronomeProcessor::getName() const { return "Metronome"; }

void MetronomeProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate_ = sampleRate;
    clickPhase_ = 0.0;
    clickSamplesRemaining_ = 0;
    lastPpqPosition_ = -1.0;
}

void MetronomeProcessor::releaseResources() {}

void MetronomeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    auto numSamples = buffer.getNumSamples();

    // If disabled, output silence
    if (!enabled.load(std::memory_order_relaxed)) {
        buffer.clear();
        return;
    }

    // Get playhead position
    auto* playHead = getPlayHead();
    if (playHead == nullptr) {
        buffer.clear();
        return;
    }

    auto posInfo = playHead->getPosition();
    if (!posInfo.hasValue()) {
        buffer.clear();
        return;
    }

    // Check if transport is playing
    auto isPlaying = posInfo->getIsPlaying();
    if (!isPlaying) {
        buffer.clear();
        lastPpqPosition_ = -1.0;
        return;
    }

    // Get BPM and PPQ
    auto bpmOpt = posInfo->getBpm();
    auto ppqOpt = posInfo->getPpqPosition();

    if (!bpmOpt.hasValue() || !ppqOpt.hasValue()) {
        buffer.clear();
        return;
    }

    double bpm = *bpmOpt;
    double ppqStart = *ppqOpt;
    double samplesPerBeat = (60.0 / bpm) * currentSampleRate_;

    // Get time signature numerator for downbeat detection
    auto timeSigOpt = posInfo->getTimeSignature();
    int timeSigNumerator = 4; // default
    if (timeSigOpt.hasValue()) {
        timeSigNumerator = timeSigOpt->numerator;
    }

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i) {
        double currentPpq = ppqStart + (static_cast<double>(i) / samplesPerBeat);
        double previousPpq = (i == 0) ? lastPpqPosition_ : ppqStart + (static_cast<double>(i - 1) / samplesPerBeat);

        // Detect beat boundary
        if (previousPpq >= 0.0 && std::floor(currentPpq) != std::floor(previousPpq)) {
            clickSamplesRemaining_ = kClickDurationSamples;
            clickPhase_ = 0.0;
            // Determine if this is a downbeat
            double beatInBar = std::fmod(currentPpq, static_cast<double>(timeSigNumerator));
            isDownbeat_ = (beatInBar < 0.5); // close to beat 0 of bar
        }

        if (clickSamplesRemaining_ > 0) {
            generateClickSample(&leftChannel[i], rightChannel ? &rightChannel[i] : nullptr);
        } else {
            leftChannel[i] = 0.0f;
            if (rightChannel) rightChannel[i] = 0.0f;
        }
    }

    // Save last PPQ for next buffer boundary detection
    lastPpqPosition_ = ppqStart + (static_cast<double>(numSamples) / samplesPerBeat);

    // Apply volume
    buffer.applyGain(volume.load(std::memory_order_relaxed));
}

void MetronomeProcessor::generateClickSample(float* left, float* right)
{
    double freq = isDownbeat_ ? kClickFrequencyHz : kClickUpbeatFrequencyHz;
    int elapsed = kClickDurationSamples - clickSamplesRemaining_;
    double amplitude = std::exp(-kClickDecayRate * static_cast<double>(elapsed));
    float sample = static_cast<float>(
        std::sin(2.0 * juce::MathConstants<double>::pi * freq * clickPhase_ / currentSampleRate_) * amplitude);

    *left = sample;
    if (right) *right = sample;

    clickPhase_ += 1.0;
    --clickSamplesRemaining_;
}

double MetronomeProcessor::getTailLengthSeconds() const { return 0.0; }
bool MetronomeProcessor::acceptsMidi() const { return false; }
bool MetronomeProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* MetronomeProcessor::createEditor() { return nullptr; }
bool MetronomeProcessor::hasEditor() const { return false; }
int MetronomeProcessor::getNumPrograms() { return 1; }
int MetronomeProcessor::getCurrentProgram() { return 0; }
void MetronomeProcessor::setCurrentProgram(int) {}
const juce::String MetronomeProcessor::getProgramName(int) { return {}; }
void MetronomeProcessor::changeProgramName(int, const juce::String&) {}
void MetronomeProcessor::getStateInformation(juce::MemoryBlock&) {}
void MetronomeProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
