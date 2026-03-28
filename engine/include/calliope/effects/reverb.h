#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace calliope {

class ReverbProcessor : public juce::AudioProcessor {
public:
    ReverbProcessor();

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

    std::atomic<float> roomSize{0.5f};
    std::atomic<float> damping{0.5f};
    std::atomic<float> wetLevel{0.33f};
    std::atomic<float> dryLevel{0.67f};
    std::atomic<float> preDelay{0.0f};   // ms
    std::atomic<bool> bypassed{false};

private:
    juce::dsp::Reverb reverb_;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayLine_{48000};
    double sampleRate_ = 44100.0;
};

} // namespace calliope
