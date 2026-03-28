#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace calliope {

class BassSynthVoice;

class BassSynthProcessor : public juce::AudioProcessor {
public:
    BassSynthProcessor();

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

    // Thread-safe MIDI injection from non-audio threads
    void addMidiEvent(const juce::MidiMessage& msg);

    // Oscillator parameters
    std::atomic<int> oscWaveform{0};        // 0=saw, 1=square, 2=sine, 3=triangle
    std::atomic<float> subOscMix{0.5f};     // 0.0-1.0 main/sub balance
    std::atomic<int> subOscOctave{-1};      // -1 or -2 octaves below

    // Filter parameters
    std::atomic<float> filterCutoff{800.0f};      // 20-5000 Hz (lower range for bass)
    std::atomic<float> filterResonance{0.2f};     // 0.0-1.0
    std::atomic<float> filterEnvAmount{0.4f};     // 0.0-1.0

    // Amp envelope
    std::atomic<float> ampAttack{0.005f};
    std::atomic<float> ampDecay{0.15f};
    std::atomic<float> ampSustain{0.9f};
    std::atomic<float> ampRelease{0.2f};

    // Filter envelope
    std::atomic<float> filterAttack{0.005f};
    std::atomic<float> filterDecay{0.3f};
    std::atomic<float> filterSustain{0.3f};
    std::atomic<float> filterRelease{0.2f};

    // Master
    std::atomic<float> masterGain{0.8f};

private:
    juce::Synthesiser synth_;
    juce::SpinLock midiLock_;
    juce::MidiBuffer pendingMidi_;
};

} // namespace calliope
