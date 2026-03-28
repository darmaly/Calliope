#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace calliope {

class DelayProcessor : public juce::AudioProcessor {
public:
    DelayProcessor();

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

    void setCurrentBpm(float bpm);

    std::atomic<float> feedback{0.3f};
    std::atomic<float> wetDry{0.5f};
    std::atomic<float> syncNoteValue{1.0f}; // 1.0=quarter, 0.5=eighth, etc.
    std::atomic<bool> pingPongEnabled{false};
    std::atomic<bool> bypassed{false};

private:
    float getDelayTimeInSamples() const;

    std::atomic<float> currentBpm_{120.0f};
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLines_[2];
    juce::dsp::DryWetMixer<float> dryWetMixer_;
    double sampleRate_ = 44100.0;
};

} // namespace calliope
