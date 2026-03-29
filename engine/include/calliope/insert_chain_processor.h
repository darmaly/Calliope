#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "calliope/insert_chain.h"

namespace calliope {

// InsertChainProcessor wraps an InsertChain as an AudioProcessor,
// making it a node in the AudioProcessorGraph. Each track and the
// master bus get their own InsertChainProcessor.
class InsertChainProcessor : public juce::AudioProcessor {
public:
    explicit InsertChainProcessor(const juce::String& trackId);

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

    // Access to the wrapped InsertChain
    InsertChain& getInsertChain();
    const InsertChain& getInsertChain() const;

    // Track identifier
    const juce::String& getTrackId() const;

    // Metering (Phase 8)
    struct MeterData {
        std::atomic<float> rmsLeft{0.0f};
        std::atomic<float> rmsRight{0.0f};
        std::atomic<float> peakLeft{0.0f};
        std::atomic<float> peakRight{0.0f};
    };
    const MeterData& getMeterData() const { return meterData_; }

private:
    juce::String trackId_;
    InsertChain chain_;
    MeterData meterData_;
};

} // namespace calliope
