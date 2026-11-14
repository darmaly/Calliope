#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "calliope/insert_chain.h"

namespace calliope {

// A mixer channel as a node in the AudioProcessorGraph: insert effects, then
// volume / pan / mute, then metering. Every track has one, and so does the
// master bus (where volume, pan and mute are simply left at their defaults).
class InsertChainProcessor : public juce::AudioProcessor {
public:
    explicit InsertChainProcessor(const juce::String& trackId);

    const juce::String getName() const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    void reset() override;

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

    // Channel controls (any thread)
    std::atomic<float> volume{1.0f};      // linear gain, 0.0 - 2.0
    std::atomic<float> pan{0.0f};         // -1.0 (left) .. +1.0 (right)
    std::atomic<bool> mute{false};
    std::atomic<bool> soloMuted{false};   // silenced because another track is soloed

    // Metering, post fader
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

    // Ramped so fader moves and mute toggles do not click.
    juce::SmoothedValue<float> gainLeft_{1.0f};
    juce::SmoothedValue<float> gainRight_{1.0f};

    void updateGainTargets();
};

} // namespace calliope
