#include "calliope/effects/parametric_eq.h"

namespace calliope {

ParametricEqProcessor::ParametricEqProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo())
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    bands[0].frequency.store(100.0f);  bands[0].q.store(0.71f); bands[0].gainDb.store(0.0f);
    bands[1].frequency.store(500.0f);  bands[1].q.store(0.71f); bands[1].gainDb.store(0.0f);
    bands[2].frequency.store(2000.0f); bands[2].q.store(0.71f); bands[2].gainDb.store(0.0f);
    bands[3].frequency.store(8000.0f); bands[3].q.store(0.71f); bands[3].gainDb.store(0.0f);
}

const juce::String ParametricEqProcessor::getName() const { return "ParametricEQ"; }

void ParametricEqProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 8192;
    spec.numChannels = 1; // per-channel filters

    for (int band = 0; band < 4; ++band)
        for (int ch = 0; ch < 2; ++ch)
            filters_[band][ch].reset();

    updateFilters();
}

void ParametricEqProcessor::releaseResources() {}

void ParametricEqProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    if (bypassed.load(std::memory_order_relaxed))
        return;

    updateFilters();

    for (int band = 0; band < 4; ++band) {
        if (!bands[band].enabled.load(std::memory_order_relaxed))
            continue;

        for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                channelData[i] = filters_[band][ch].processSample(channelData[i]);
            }
        }
    }
}

void ParametricEqProcessor::updateFilters()
{
    for (int band = 0; band < 4; ++band) {
        float freq = bands[band].frequency.load(std::memory_order_relaxed);
        float q = bands[band].q.load(std::memory_order_relaxed);
        float gainDb = bands[band].gainDb.load(std::memory_order_relaxed);
        float gainFactor = juce::Decibels::decibelsToGain(gainDb);

        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> newCoeffs;

        switch (band) {
            case 0: // Low shelf
                newCoeffs = new juce::dsp::IIR::Coefficients<float>(
                    juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
                        sampleRate_, freq, q, gainFactor));
                break;
            case 1: // Peak 1
            case 2: // Peak 2
                newCoeffs = new juce::dsp::IIR::Coefficients<float>(
                    juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
                        sampleRate_, freq, q, gainFactor));
                break;
            case 3: // High shelf
                newCoeffs = new juce::dsp::IIR::Coefficients<float>(
                    juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
                        sampleRate_, freq, q, gainFactor));
                break;
        }

        if (newCoeffs != nullptr) {
            for (int ch = 0; ch < 2; ++ch)
                *filters_[band][ch].coefficients = *newCoeffs;
        }
    }
}

double ParametricEqProcessor::getTailLengthSeconds() const { return 0.0; }
bool ParametricEqProcessor::acceptsMidi() const { return false; }
bool ParametricEqProcessor::producesMidi() const { return false; }

juce::AudioProcessorEditor* ParametricEqProcessor::createEditor() { return nullptr; }
bool ParametricEqProcessor::hasEditor() const { return false; }
int ParametricEqProcessor::getNumPrograms() { return 1; }
int ParametricEqProcessor::getCurrentProgram() { return 0; }
void ParametricEqProcessor::setCurrentProgram(int) {}
const juce::String ParametricEqProcessor::getProgramName(int) { return {}; }
void ParametricEqProcessor::changeProgramName(int, const juce::String&) {}
void ParametricEqProcessor::getStateInformation(juce::MemoryBlock&) {}
void ParametricEqProcessor::setStateInformation(const void*, int) {}

} // namespace calliope
