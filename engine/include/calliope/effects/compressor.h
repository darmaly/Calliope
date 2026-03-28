#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace calliope {

class CompressorProcessor : public juce::AudioProcessor {
public:
    CompressorProcessor();

    const juce::String getName() const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    std::atomic<float> threshold{-10.0f};  // dB
    std::atomic<float> ratio{4.0f};
    std::atomic<float> attack{10.0f};      // ms
    std::atomic<float> release{100.0f};    // ms
    std::atomic<float> makeupGain{0.0f};   // dB
    std::atomic<bool> bypassed{false};

private:
    juce::dsp::Compressor<float> compressor_;
};

} // namespace calliope
