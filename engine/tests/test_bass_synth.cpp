#include <catch2/catch_test_macros.hpp>
#include "calliope/instruments/bass_synth.h"
#include <cmath>

using namespace calliope;

namespace {

float computeRMS(const juce::AudioBuffer<float>& buffer, int channel = 0)
{
    float sum = 0.0f;
    auto* data = buffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        sum += data[i] * data[i];
    return std::sqrt(sum / static_cast<float>(buffer.getNumSamples()));
}

bool hasNonZeroSamples(const juce::AudioBuffer<float>& buffer, int channel = 0)
{
    auto* data = buffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        if (data[i] != 0.0f) return true;
    return false;
}

} // namespace

TEST_CASE("BassSynth produces non-zero audio on note-on", "[BassSynth]")
{
    BassSynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 36, 0.8f)); // low C

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE(hasNonZeroSamples(buffer, 0));
    REQUIRE(hasNonZeroSamples(buffer, 1));
    REQUIRE(computeRMS(buffer) > 0.0f);
}

TEST_CASE("BassSynth sub-oscillator contributes to output", "[BassSynth]")
{
    // With sub-oscillator mix at 0
    auto getRMSWithSubMix = [](float subMix) {
        BassSynthProcessor synth;
        synth.subOscMix.store(subMix);
        synth.filterCutoff.store(5000.0f); // wide open for bass
        synth.filterEnvAmount.store(0.0f);
        synth.oscWaveform.store(2); // sine for clean comparison
        synth.prepareToPlay(44100.0, 512);

        synth.addMidiEvent(juce::MidiMessage::noteOn(1, 36, 0.8f));

        // Process several blocks for stability
        float totalRMS = 0.0f;
        for (int b = 0; b < 10; ++b) {
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;
            synth.processBlock(buffer, midi);
            totalRMS += computeRMS(buffer);
        }
        return totalRMS / 10.0f;
    };

    float noSubRMS = getRMSWithSubMix(0.0f);
    float withSubRMS = getRMSWithSubMix(0.5f);

    // Both should produce audio
    REQUIRE(noSubRMS > 0.0f);
    REQUIRE(withSubRMS > 0.0f);

    // Sub-oscillator at different frequency should change the RMS
    // (sine at fundamental vs sine+sine-at-half-freq will have different RMS)
    REQUIRE(noSubRMS != withSubRMS);
}

TEST_CASE("BassSynth low polyphony with voice stealing", "[BassSynth]")
{
    BassSynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    // Inject 5 notes (more than 4 voices)
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 36, 0.8f));
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 40, 0.8f));
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 43, 0.8f));
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 48, 0.8f));
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 52, 0.8f));

    // Should still produce output (voice stealing handles overflow)
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE(hasNonZeroSamples(buffer, 0));
    REQUIRE(computeRMS(buffer) > 0.0f);
}

TEST_CASE("BassSynth filter with low cutoff reduces high-freq content", "[BassSynth]")
{
    auto getRMSWithCutoff = [](float cutoff) {
        BassSynthProcessor synth;
        synth.oscWaveform.store(0); // saw - rich in harmonics
        synth.filterCutoff.store(cutoff);
        synth.filterEnvAmount.store(0.0f);
        synth.prepareToPlay(44100.0, 512);

        synth.addMidiEvent(juce::MidiMessage::noteOn(1, 48, 0.8f));

        float rms = 0.0f;
        for (int b = 0; b < 5; ++b) {
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;
            synth.processBlock(buffer, midi);
            rms = computeRMS(buffer);
        }
        return rms;
    };

    float lowCutoffRMS = getRMSWithCutoff(100.0f);
    float highCutoffRMS = getRMSWithCutoff(5000.0f);

    // Low cutoff should attenuate harmonics, reducing RMS
    REQUIRE(lowCutoffRMS < highCutoffRMS);
}

TEST_CASE("BassSynth output is zero when no note is active", "[BassSynth]")
{
    BassSynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE_FALSE(hasNonZeroSamples(buffer, 0));
    REQUIRE_FALSE(hasNonZeroSamples(buffer, 1));
}
