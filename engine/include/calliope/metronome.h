#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace calliope {

class MetronomeProcessor : public juce::AudioProcessor {
public:
    MetronomeProcessor();

    const juce::String getName() const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;

    // Editor stubs
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    std::atomic<bool> enabled{true};
    std::atomic<float> volume{0.7f};

private:
    double currentSampleRate_ = 44100.0;
    double clickPhase_ = 0.0;
    int clickSamplesRemaining_ = 0;
    double lastPpqPosition_ = -1.0;

    // Click sound parameters
    static constexpr double kClickFrequencyHz = 1000.0;      // downbeat click frequency
    static constexpr double kClickUpbeatFrequencyHz = 800.0;  // upbeat click frequency
    static constexpr int kClickDurationSamples = 1000;        // ~23ms at 44.1kHz
    static constexpr double kClickDecayRate = 0.005;          // exponential decay

    bool isDownbeat_ = false;
    void generateClickSample(float* left, float* right);
};

} // namespace calliope
