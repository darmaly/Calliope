#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace calliope {

enum class TransportState { Stopped, Playing, Paused };

class Transport : public juce::AudioPlayHead {
public:
    Transport();

    // AudioPlayHead interface
    juce::Optional<PositionInfo> getPosition() const override;

    // Audio thread: call each processBlock to advance position
    void advancePosition(int numSamples);

    // UI thread setters (lock-free via atomics)
    void play();
    void stop();
    void pause();
    void setBpm(double newBpm);
    void setTimeSignature(int numerator, int denominator);
    void setLoopRegion(double startBeat, double endBeat, bool enabled);
    void setSampleRate(double newSampleRate);

    // UI thread getters
    TransportState getState() const;
    double getBpm() const;
    int getTimeSignatureNumerator() const;
    int getTimeSignatureDenominator() const;
    int64_t getSamplePosition() const;
    double getPpqPosition() const;
    bool isLooping() const;
    double getLoopStartBeat() const;
    double getLoopEndBeat() const;

private:
    std::atomic<int64_t> samplePosition_{0};
    std::atomic<double> bpm_{120.0};
    std::atomic<int> timeSigNumerator_{4};
    std::atomic<int> timeSigDenominator_{4};
    std::atomic<TransportState> state_{TransportState::Stopped};
    std::atomic<bool> looping_{false};
    std::atomic<double> loopStartBeat_{0.0};
    std::atomic<double> loopEndBeat_{0.0};
    std::atomic<double> sampleRate_{44100.0};
};

} // namespace calliope
