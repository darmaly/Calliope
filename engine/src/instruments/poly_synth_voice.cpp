#include "calliope/instruments/poly_synth_voice.h"
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/poly_synth_sound.h"
#include <cmath>

namespace calliope {

PolySynthVoice::PolySynthVoice(PolySynthProcessor& parent)
    : parent_(parent)
{
}

bool PolySynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<PolySynthSound*>(sound) != nullptr;
}

void PolySynthVoice::startNote(int midiNoteNumber, float velocity,
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
    osc1Phase_ = 0.0;
    osc2Phase_ = 0.0;
    lfoPhase_ = 0.0f;

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

void PolySynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
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

void PolySynthVoice::pitchWheelMoved(int) {}
void PolySynthVoice::controllerMoved(int, int) {}

void PolySynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                      int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
        return;

    double osc1PhaseInc = currentFrequency_ / sampleRate;

    // Osc2 with detune (cents to frequency ratio)
    float detuneCents = parent_.osc2Detune.load(std::memory_order_relaxed);
    double osc2Freq = currentFrequency_ * std::pow(2.0, detuneCents / 1200.0);
    double osc2PhaseInc = osc2Freq / sampleRate;

    int waveform1 = parent_.osc1Waveform.load(std::memory_order_relaxed);
    int waveform2 = parent_.osc2Waveform.load(std::memory_order_relaxed);
    float mix = parent_.oscMix.load(std::memory_order_relaxed);

    float baseCutoff = parent_.filterCutoff.load(std::memory_order_relaxed);
    float resonance = parent_.filterResonance.load(std::memory_order_relaxed);
    float filterEnvAmt = parent_.filterEnvAmount.load(std::memory_order_relaxed);

    float lfoRateHz = parent_.lfoRate.load(std::memory_order_relaxed);
    float lfoDepthVal = parent_.lfoDepth.load(std::memory_order_relaxed);
    int lfoTargetVal = parent_.lfoTarget.load(std::memory_order_relaxed);
    float lfoPhaseInc = lfoRateHz / static_cast<float>(sampleRate);

    filter_.setResonance(resonance);

    for (int i = startSample; i < startSample + numSamples; ++i) {
        float ampEnvValue = ampEnvelope_.getNextSample();
        if (!ampEnvelope_.isActive()) {
            clearCurrentNote();
            break;
        }

        float filterEnvValue = filterEnvelope_.getNextSample();

        // LFO
        float lfoValue = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase_) * lfoDepthVal;
        lfoPhase_ += lfoPhaseInc;
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;

        // Apply LFO to pitch if target is pitch
        double effectiveOsc1PhaseInc = osc1PhaseInc;
        double effectiveOsc2PhaseInc = osc2PhaseInc;
        if (lfoTargetVal == 0) {
            // Pitch modulation: +/- 1 semitone at full depth
            double pitchMod = std::pow(2.0, static_cast<double>(lfoValue) / 12.0);
            effectiveOsc1PhaseInc *= pitchMod;
            effectiveOsc2PhaseInc *= pitchMod;
        }

        // Generate oscillator samples
        float osc1Sample = generateOscSample(waveform1, osc1Phase_, effectiveOsc1PhaseInc);
        float osc2Sample = generateOscSample(waveform2, osc2Phase_, effectiveOsc2PhaseInc);

        // Mix oscillators
        float sample = osc1Sample * (1.0f - mix) + osc2Sample * mix;

        // Filter envelope modulation
        float envModulatedCutoff = baseCutoff + filterEnvAmt * filterEnvValue * 10000.0f;

        // LFO filter modulation
        if (lfoTargetVal == 1) {
            envModulatedCutoff += lfoValue * 5000.0f;
        }

        // Clamp cutoff
        envModulatedCutoff = juce::jlimit(20.0f, 20000.0f, envModulatedCutoff);
        filter_.setCutoffFrequencyHz(envModulatedCutoff);

        // Process through filter (single sample via AudioBlock)
        float sampleData[1] = { sample };
        float* channelPtrs[1] = { sampleData };
        juce::dsp::AudioBlock<float> singleBlock(channelPtrs, 1, 1);
        juce::dsp::ProcessContextReplacing<float> context(singleBlock);
        filter_.process(context);
        sample = sampleData[0];

        // Apply amp envelope and LFO amp modulation
        float ampMod = 1.0f;
        if (lfoTargetVal == 2) {
            ampMod = 1.0f + lfoValue * 0.5f; // +/- 50% at full depth
            ampMod = juce::jlimit(0.0f, 2.0f, ampMod);
        }

        sample *= ampEnvValue * level_ * ampMod;

        // Write stereo output
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, i, sample);
    }
}

float PolySynthVoice::generateOscSample(int waveform, double& phase, double phaseIncrement)
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
            sample = std::sin(2.0 * juce::MathConstants<double>::pi * phase);
            break;
        }
        case 3: { // Triangle (integrated square with PolyBLEP)
            float naive = 2.0f * std::fabs(2.0f * t - 1.0f) - 1.0f;
            sample = naive;
            break;
        }
        default:
            sample = 0.0f;
            break;
    }

    // Advance phase
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample;
}

float PolySynthVoice::polyBlep(float t, float dt)
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
