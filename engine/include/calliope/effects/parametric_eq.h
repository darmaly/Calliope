#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace calliope {

class ParametricEqProcessor : public juce::AudioProcessor {
public:
    ParametricEqProcessor();

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

    struct BandParams {
        std::atomic<float> frequency{100.0f};
        std::atomic<float> q{0.71f};
        std::atomic<float> gainDb{0.0f};
        std::atomic<bool> enabled{true};
    };

    BandParams bands[4]; // [0]=lowShelf, [1]=peak1, [2]=peak2, [3]=highShelf
    std::atomic<bool> bypassed{false};

private:
    void updateFilters();

    // 4 bands x 2 channels
    juce::dsp::IIR::Filter<float> filters_[4][2];
    double sampleRate_ = 44100.0;
};

} // namespace calliope
