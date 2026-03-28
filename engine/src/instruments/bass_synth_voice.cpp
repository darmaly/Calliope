#include "calliope/instruments/bass_synth_voice.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/bass_synth_sound.h"
#include <cmath>

namespace calliope {

BassSynthVoice::BassSynthVoice(BassSynthProcessor& parent)
    : parent_(parent)
{
}

bool BassSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<BassSynthSound*>(sound) != nullptr;
}

void BassSynthVoice::startNote(int midiNoteNumber, float velocity,
                                juce::SynthesiserSound*, int)
{
    // Read ADSR params from parent atomics at note-on time
    juce::ADSR::Parameters ampParams;
    ampParams.attack  = parent_.ampAttack.load(std::memory_order_relaxed);
    ampParams.decay   = parent_.ampDecay.load(std::memory_order_relaxed);
    ampParams.sustain = parent_.ampSustain.load(std::memory_order_relaxed);
    ampParams.release = parent_.ampRelease.load(std::memory_order_relaxed);
    ampEnvelope_.setParameters(ampParams);
    ampEnvelope_.setSampleRate(getSampleRate());
    ampEnvelope_.noteOn();

    juce::ADSR::Parameters filterParams;
    filterParams.attack  = parent_.filterAttack.load(std::memory_order_relaxed);
    filterParams.decay   = parent_.filterDecay.load(std::memory_order_relaxed);
    filterParams.sustain = parent_.filterSustain.load(std::memory_order_relaxed);
    filterParams.release = parent_.filterRelease.load(std::memory_order_relaxed);
    filterEnvelope_.setParameters(filterParams);
    filterEnvelope_.setSampleRate(getSampleRate());
    filterEnvelope_.noteOn();

    currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    level_ = velocity;
    mainOscPhase_ = 0.0;
    subOscPhase_ = 0.0;

    // Prepare filter for this voice
    filter_.reset();
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = getSampleRate();
    spec.maximumBlockSize = 1;
    spec.numChannels = 1;
    filter_.prepare(spec);
    filter_.setMode(juce::dsp::LadderFilterMode::LPF24);
    filter_.setResonance(parent_.filterResonance.load(std::memory_order_relaxed));
}

void BassSynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff) {
        ampEnvelope_.noteOff();
        filterEnvelope_.noteOff();
    } else {
        ampEnvelope_.reset();
        filterEnvelope_.reset();
        clearCurrentNote();
    }
}

void BassSynthVoice::pitchWheelMoved(int) {}
void BassSynthVoice::controllerMoved(int, int) {}

void BassSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                      int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
        return;

    double mainPhaseInc = currentFrequency_ / sampleRate;

    // Sub-oscillator frequency: one or two octaves below
    int subOctave = parent_.subOscOctave.load(std::memory_order_relaxed);
    double subFreq = currentFrequency_ * std::pow(2.0, static_cast<double>(subOctave));
    double subPhaseInc = subFreq / sampleRate;

    int waveform = parent_.oscWaveform.load(std::memory_order_relaxed);
    float subMix = parent_.subOscMix.load(std::memory_order_relaxed);

    float baseCutoff = parent_.filterCutoff.load(std::memory_order_relaxed);
    float resonance = parent_.filterResonance.load(std::memory_order_relaxed);
    float filterEnvAmt = parent_.filterEnvAmount.load(std::memory_order_relaxed);

    filter_.setResonance(resonance);

    for (int i = startSample; i < startSample + numSamples; ++i) {
        float ampEnvValue = ampEnvelope_.getNextSample();
        if (!ampEnvelope_.isActive()) {
            clearCurrentNote();
            break;
        }

        float filterEnvValue = filterEnvelope_.getNextSample();

        // Generate main oscillator using PolyBLEP
        float mainSample = generateOscSample(waveform, mainOscPhase_, mainPhaseInc);

        // Sub-oscillator: always sine for clean low end
        float subSample = static_cast<float>(
            std::sin(2.0 * juce::MathConstants<double>::pi * subOscPhase_));
        subOscPhase_ += subPhaseInc;
        if (subOscPhase_ >= 1.0)
            subOscPhase_ -= 1.0;

        // Mix main and sub oscillators
        float sample = mainSample * (1.0f - subMix) + subSample * subMix;

        // Filter envelope modulation
        float envModulatedCutoff = baseCutoff + filterEnvAmt * filterEnvValue * 5000.0f;
        envModulatedCutoff = juce::jlimit(20.0f, 20000.0f, envModulatedCutoff);
        filter_.setCutoffFrequencyHz(envModulatedCutoff);

        // Process through filter (single sample via AudioBlock)
        float sampleData[1] = { sample };
        float* channelPtrs[1] = { sampleData };
        juce::dsp::AudioBlock<float> singleBlock(channelPtrs, 1, 1);
        juce::dsp::ProcessContextReplacing<float> context(singleBlock);
        filter_.process(context);
        sample = sampleData[0];

        // Apply amp envelope
        sample *= ampEnvValue * level_;

        // Write stereo output
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, i, sample);
    }
}

float BassSynthVoice::generateOscSample(int waveform, double& phase, double phaseIncrement)
{
    float t = static_cast<float>(phase);
    float dt = static_cast<float>(phaseIncrement);
    float sample = 0.0f;

    switch (waveform) {
        case 0: { // Saw
            float naive = 2.0f * t - 1.0f;
            sample = naive - polyBlep(t, dt);
            break;
        }
        case 1: { // Square
            float naive = (t < 0.5f) ? 1.0f : -1.0f;
            sample = naive + polyBlep(t, dt) - polyBlep(std::fmod(t + 0.5f, 1.0f), dt);
            break;
        }
        case 2: { // Sine
            sample = static_cast<float>(
                std::sin(2.0 * juce::MathConstants<double>::pi * phase));
            break;
        }
        case 3: { // Triangle
            sample = 2.0f * std::fabs(2.0f * t - 1.0f) - 1.0f;
            break;
        }
        default:
            sample = 0.0f;
            break;
    }

    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample;
}

float BassSynthVoice::polyBlep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

} // namespace calliope
