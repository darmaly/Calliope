#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace calliope {

class BassSynthProcessor;

class BassSynthVoice : public juce::SynthesiserVoice {
public:
    explicit BassSynthVoice(BassSynthProcessor& parent);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                         int startSample, int numSamples) override;

private:
    float generateOscSample(int waveform, double& phase, double phaseIncrement);
    static float polyBlep(float t, float dt);

    BassSynthProcessor& parent_;

    juce::ADSR ampEnvelope_;
    juce::ADSR filterEnvelope_;
    juce::dsp::LadderFilter<float> filter_;

    double mainOscPhase_ = 0.0;
    double subOscPhase_ = 0.0;
    double currentFrequency_ = 440.0;
    float level_ = 0.0f;
};

} // namespace calliope
