#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <array>
#include <vector>

namespace calliope {

class DrumMachineProcessor : public juce::AudioProcessor {
public:
    static constexpr int kNumPads = 16;
    static constexpr int kFirstMidiNote = 36; // C1, GM kick drum

    DrumMachineProcessor();

    // Sample management
    bool loadSample(int padIndex, const juce::File& file);
    void clearSample(int padIndex);
    juce::String getSampleName(int padIndex) const;
    std::vector<juce::String> getSampleNames() const;

    // Thread-safe MIDI injection
    void addMidiEvent(const juce::MidiMessage& msg);

    // AudioProcessor overrides
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

    std::atomic<float> volume{0.8f};

private:
    juce::Synthesiser synth_;
    juce::AudioFormatManager formatManager_;
    juce::SpinLock midiLock_;
    juce::MidiBuffer pendingMidi_;
    std::array<juce::String, kNumPads> padNames_;
};

} // namespace calliope

// Make DrumMachineProcessor available without namespace qualification in tests
using calliope::DrumMachineProcessor;
