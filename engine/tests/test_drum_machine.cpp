#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/instruments/drum_machine.h"
#include <cmath>

using namespace calliope;

// Helper: create a temporary WAV file with a sine wave for testing
static juce::File createTestWavFile(juce::TemporaryFile& tempFile)
{
    auto file = tempFile.getFile();
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            new juce::FileOutputStream(file), 44100.0, 1, 16, {}, 0));
    if (writer) {
        juce::AudioBuffer<float> testBuffer(1, 4410); // 100ms at 44.1kHz
        for (int i = 0; i < 4410; ++i)
            testBuffer.setSample(0, i,
                std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * i / 44100.0));
        writer->writeFromAudioSampleBuffer(testBuffer, 0, 4410);
    }
    return file;
}

// Helper: check if buffer has any non-zero samples
static bool bufferHasAudio(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (std::abs(buffer.getSample(ch, i)) > 1e-7f)
                return true;
    return false;
}

TEST_CASE("DrumMachine produces zero output with no samples and no MIDI", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    dm.processBlock(buffer, midi);

    REQUIRE_FALSE(bufferHasAudio(buffer));
}

TEST_CASE("DrumMachine produces audio when sample loaded and note-on triggered", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    juce::TemporaryFile tempFile(".wav");
    auto wavFile = createTestWavFile(tempFile);
    REQUIRE(dm.loadSample(0, wavFile));

    // Inject note-on for pad 0 (MIDI note 36)
    dm.addMidiEvent(juce::MidiMessage::noteOn(1, DrumMachineProcessor::kFirstMidiNote, 1.0f));

    // Process several blocks to capture audio output
    bool foundAudio = false;
    for (int block = 0; block < 20; ++block) {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        dm.processBlock(buffer, midi);
        if (bufferHasAudio(buffer)) {
            foundAudio = true;
            break;
        }
    }
    REQUIRE(foundAudio);
}

TEST_CASE("DrumMachine produces zero output for note-on on unloaded pad", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    // No sample loaded, but trigger note-on for pad 5 (MIDI note 41)
    dm.addMidiEvent(juce::MidiMessage::noteOn(1, DrumMachineProcessor::kFirstMidiNote + 5, 1.0f));

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    dm.processBlock(buffer, midi);

    REQUIRE_FALSE(bufferHasAudio(buffer));
}

TEST_CASE("DrumMachine pad index maps to correct MIDI note (GM drum standard)", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    juce::TemporaryFile tempFile(".wav");
    auto wavFile = createTestWavFile(tempFile);

    // Load sample to pad 3 -> should be mapped to MIDI note 39
    REQUIRE(dm.loadSample(3, wavFile));

    // Trigger MIDI note 39 (kFirstMidiNote + 3)
    dm.addMidiEvent(juce::MidiMessage::noteOn(1, DrumMachineProcessor::kFirstMidiNote + 3, 1.0f));

    bool foundAudio = false;
    for (int block = 0; block < 20; ++block) {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        dm.processBlock(buffer, midi);
        if (bufferHasAudio(buffer)) {
            foundAudio = true;
            break;
        }
    }
    REQUIRE(foundAudio);
}

TEST_CASE("DrumMachine volume=0 produces silence even with sample playing", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    juce::TemporaryFile tempFile(".wav");
    auto wavFile = createTestWavFile(tempFile);
    REQUIRE(dm.loadSample(0, wavFile));

    dm.volume.store(0.0f);
    dm.addMidiEvent(juce::MidiMessage::noteOn(1, DrumMachineProcessor::kFirstMidiNote, 1.0f));

    bool foundAudio = false;
    for (int block = 0; block < 20; ++block) {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        dm.processBlock(buffer, midi);
        if (bufferHasAudio(buffer)) {
            foundAudio = true;
            break;
        }
    }
    REQUIRE_FALSE(foundAudio);
}

TEST_CASE("DrumMachine getSampleNames returns loaded sample names", "[DrumMachine]")
{
    DrumMachineProcessor dm;

    // Initially all empty
    auto names = dm.getSampleNames();
    REQUIRE(names.size() == DrumMachineProcessor::kNumPads);
    for (auto& n : names)
        REQUIRE(n.isEmpty());

    juce::TemporaryFile tempFile(".wav");
    auto wavFile = createTestWavFile(tempFile);
    REQUIRE(dm.loadSample(0, wavFile));

    REQUIRE_FALSE(dm.getSampleName(0).isEmpty());

    // Pad 1 should still be empty
    REQUIRE(dm.getSampleName(1).isEmpty());
}

TEST_CASE("DrumMachine rejects invalid pad indices", "[DrumMachine]")
{
    DrumMachineProcessor dm;

    juce::TemporaryFile tempFile(".wav");
    auto wavFile = createTestWavFile(tempFile);

    REQUIRE_FALSE(dm.loadSample(-1, wavFile));
    REQUIRE_FALSE(dm.loadSample(16, wavFile));
    REQUIRE_FALSE(dm.loadSample(100, wavFile));
}

TEST_CASE("DrumMachine clearSample removes loaded sample", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    juce::TemporaryFile tempFile(".wav");
    auto wavFile = createTestWavFile(tempFile);
    REQUIRE(dm.loadSample(0, wavFile));
    REQUIRE_FALSE(dm.getSampleName(0).isEmpty());

    dm.clearSample(0);
    REQUIRE(dm.getSampleName(0).isEmpty());

    // After clearing, triggering should produce no audio
    dm.addMidiEvent(juce::MidiMessage::noteOn(1, DrumMachineProcessor::kFirstMidiNote, 1.0f));

    bool foundAudio = false;
    for (int block = 0; block < 10; ++block) {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        dm.processBlock(buffer, midi);
        if (bufferHasAudio(buffer)) {
            foundAudio = true;
            break;
        }
    }
    REQUIRE_FALSE(foundAudio);
}

TEST_CASE("DrumMachine accepts MIDI and does not produce MIDI", "[DrumMachine]")
{
    DrumMachineProcessor dm;
    REQUIRE(dm.acceptsMidi() == true);
    REQUIRE(dm.producesMidi() == false);
}

TEST_CASE("DrumMachine has correct constants", "[DrumMachine]")
{
    REQUIRE(DrumMachineProcessor::kNumPads == 16);
    REQUIRE(DrumMachineProcessor::kFirstMidiNote == 36);
}
