#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace calliope {

// Pass-through processor for the master bus.
// In Phase 5, this will host the insert effect chain.
// For now it applies master volume and passes audio through.
class MasterBusProcessor : public juce::AudioProcessor {
public:
    MasterBusProcessor();

    const juce::String getName() const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;

    // Editor stubs (required by AudioProcessor but unused)
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    std::atomic<float> masterVolume{1.0f};

    // Metering (Phase 8)
    struct MeterData {
        std::atomic<float> rmsLeft{0.0f};
        std::atomic<float> rmsRight{0.0f};
        std::atomic<float> peakLeft{0.0f};
        std::atomic<float> peakRight{0.0f};
    };
    const MeterData& getMeterData() const { return meterData_; }

private:
    MeterData meterData_;
};

} // namespace calliope
