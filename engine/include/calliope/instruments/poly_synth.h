#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace calliope {

class PolySynthVoice;

class PolySynthProcessor : public juce::AudioProcessor {
public:
    PolySynthProcessor();

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
    std::atomic<int> osc1Waveform{0};       // 0=saw, 1=square, 2=sine, 3=triangle
    std::atomic<int> osc2Waveform{2};       // default sine
    std::atomic<float> oscMix{0.5f};        // 0.0-1.0 balance osc1/osc2
    std::atomic<float> osc2Detune{0.0f};    // -100 to +100 cents

    // Filter parameters
    std::atomic<float> filterCutoff{5000.0f};     // 20-20000 Hz
    std::atomic<float> filterResonance{0.1f};     // 0.0-1.0
    std::atomic<float> filterEnvAmount{0.3f};     // 0.0-1.0

    // Amp envelope
    std::atomic<float> ampAttack{0.01f};    // seconds
    std::atomic<float> ampDecay{0.1f};
    std::atomic<float> ampSustain{0.8f};
    std::atomic<float> ampRelease{0.3f};

    // Filter envelope
    std::atomic<float> filterAttack{0.01f};
    std::atomic<float> filterDecay{0.2f};
    std::atomic<float> filterSustain{0.5f};
    std::atomic<float> filterRelease{0.3f};

    // LFO
    std::atomic<float> lfoRate{1.0f};       // 0.1-20 Hz
    std::atomic<float> lfoDepth{0.0f};      // 0.0-1.0
    std::atomic<int> lfoTarget{1};          // 0=pitch, 1=filter, 2=amp

    // Master
    std::atomic<float> masterGain{0.7f};    // 0.0-1.0

private:
    juce::Synthesiser synth_;
    juce::SpinLock midiLock_;
    juce::MidiBuffer pendingMidi_;
};

} // namespace calliope
