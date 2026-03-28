#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/instruments/poly_synth.h"
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

TEST_CASE("PolySynth produces non-zero audio on note-on", "[PolySynth]")
{
    PolySynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    // Inject note-on
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f));

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE(hasNonZeroSamples(buffer, 0));
    REQUIRE(hasNonZeroSamples(buffer, 1));
    REQUIRE(computeRMS(buffer) > 0.0f);
}

TEST_CASE("PolySynth output is zero when no note is active", "[PolySynth]")
{
    PolySynthProcessor synth;
    synth.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    REQUIRE_FALSE(hasNonZeroSamples(buffer, 0));
    REQUIRE_FALSE(hasNonZeroSamples(buffer, 1));
}

TEST_CASE("PolySynth amp envelope release decays to zero after noteOff", "[PolySynth]")
{
    PolySynthProcessor synth;
    synth.ampRelease.store(0.01f); // very short release
    synth.prepareToPlay(44100.0, 512);

    // Note on
    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f));
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    synth.processBlock(buffer, midi);

    // Note off
    synth.addMidiEvent(juce::MidiMessage::noteOff(1, 60));
    juce::AudioBuffer<float> buffer2(2, 512);
    buffer2.clear();
    juce::MidiBuffer midi2;
    synth.processBlock(buffer2, midi2);

    // Process several more blocks to let release finish
    for (int b = 0; b < 5; ++b) {
        juce::AudioBuffer<float> buf(2, 512);
        buf.clear();
        juce::MidiBuffer m;
        synth.processBlock(buf, m);
    }

    // Final block should be silent (release is 10ms = ~441 samples)
    juce::AudioBuffer<float> finalBuf(2, 512);
    finalBuf.clear();
    juce::MidiBuffer finalMidi;
    synth.processBlock(finalBuf, finalMidi);

    REQUIRE(computeRMS(finalBuf) < 0.001f);
}

TEST_CASE("PolySynth waveform selection produces different output", "[PolySynth]")
{
    auto getBufferForWaveform = [](int waveform) {
        PolySynthProcessor synth;
        synth.osc1Waveform.store(waveform);
        synth.osc2Waveform.store(waveform);
        synth.prepareToPlay(44100.0, 512);

        synth.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f));
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        synth.processBlock(buffer, midi);
        return computeRMS(buffer);
    };

    float sawRMS = getBufferForWaveform(0);
    float squareRMS = getBufferForWaveform(1);
    float sineRMS = getBufferForWaveform(2);
    float triRMS = getBufferForWaveform(3);

    // All waveforms should produce non-zero output
    REQUIRE(sawRMS > 0.0f);
    REQUIRE(squareRMS > 0.0f);
    REQUIRE(sineRMS > 0.0f);
    REQUIRE(triRMS > 0.0f);

    // Different waveforms should have different RMS values (at least some)
    bool allSame = (sawRMS == squareRMS) && (squareRMS == sineRMS) && (sineRMS == triRMS);
    REQUIRE_FALSE(allSame);
}

TEST_CASE("PolySynth filter cutoff modifies the spectrum", "[PolySynth]")
{
    auto getRMSWithCutoff = [](float cutoff) {
        PolySynthProcessor synth;
        synth.osc1Waveform.store(0); // saw - rich in harmonics
        synth.filterCutoff.store(cutoff);
        synth.filterEnvAmount.store(0.0f); // no env modulation
        synth.prepareToPlay(44100.0, 512);

        synth.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f));

        // Process several blocks for filter to stabilize
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

    float lowCutoffRMS = getRMSWithCutoff(200.0f);
    float highCutoffRMS = getRMSWithCutoff(18000.0f);

    // Low cutoff should reduce output energy compared to wide open
    REQUIRE(lowCutoffRMS < highCutoffRMS);
}

TEST_CASE("PolySynth LFO with filter target produces amplitude variation", "[PolySynth]")
{
    PolySynthProcessor synth;
    synth.lfoRate.store(10.0f);      // fast LFO
    synth.lfoDepth.store(1.0f);      // full depth
    synth.lfoTarget.store(1);        // target filter
    synth.filterCutoff.store(2000.0f);
    synth.filterEnvAmount.store(0.0f);
    synth.osc1Waveform.store(0);     // saw
    synth.prepareToPlay(44100.0, 512);

    synth.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f));

    // Collect RMS over multiple blocks
    std::vector<float> rmsValues;
    for (int b = 0; b < 20; ++b) {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        synth.processBlock(buffer, midi);
        rmsValues.push_back(computeRMS(buffer));
    }

    // Find min and max RMS - LFO should cause variation
    float minRMS = *std::min_element(rmsValues.begin(), rmsValues.end());
    float maxRMS = *std::max_element(rmsValues.begin(), rmsValues.end());

    REQUIRE(maxRMS > minRMS * 1.05f); // at least 5% variation
}

TEST_CASE("PolySynth supports polyphonic output with multiple notes", "[PolySynth]")
{
    // Use sine waves and wide-open filter to avoid phase/filter interactions
    // Process several blocks to get past initial transients

    auto getAvgRMS = [](int noteCount) {
        PolySynthProcessor synth;
        synth.osc1Waveform.store(2); // sine - clean, no harmonics
        synth.oscMix.store(0.0f);    // only osc1
        synth.filterCutoff.store(20000.0f);
        synth.filterEnvAmount.store(0.0f);
        synth.prepareToPlay(44100.0, 512);

        // Add notes at well-separated frequencies
        synth.addMidiEvent(juce::MidiMessage::noteOn(1, 48, 0.8f)); // C3
        if (noteCount >= 2)
            synth.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f)); // C4
        if (noteCount >= 3)
            synth.addMidiEvent(juce::MidiMessage::noteOn(1, 72, 0.8f)); // C5

        // Process several blocks and average RMS
        float totalRMS = 0.0f;
        int blockCount = 10;
        for (int b = 0; b < blockCount; ++b) {
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;
            synth.processBlock(buffer, midi);
            totalRMS += computeRMS(buffer);
        }
        return totalRMS / static_cast<float>(blockCount);
    };

    float singleRMS = getAvgRMS(1);
    float tripleRMS = getAvgRMS(3);

    // Three notes should produce more output energy than one
    REQUIRE(tripleRMS > singleRMS * 1.1f);
}
