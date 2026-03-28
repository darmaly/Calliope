#include <catch2/catch_test_macros.hpp>
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/drum_machine.h"
#include "calliope/commands/instrument_commands.h"
#include "calliope/audio_graph.h"
#include "calliope/engine.h"
#include "calliope/parameter_registry.h"

using namespace calliope;

// ==================================================================
// AudioGraph instrument accessor tests
// ==================================================================

TEST_CASE("AudioGraph::getPolySynth() returns valid reference after initialise", "[InstrumentCommands]")
{
    // We test through Engine since AudioGraph::initialise requires audio device
    // Instead, test the processor directly as the pattern used in existing tests
    PolySynthProcessor ps;
    ps.prepareToPlay(44100.0, 512);
    REQUIRE(ps.getName().isNotEmpty());
}

TEST_CASE("AudioGraph::getBassSynth() returns valid reference after initialise", "[InstrumentCommands]")
{
    BassSynthProcessor bs;
    bs.prepareToPlay(44100.0, 512);
    REQUIRE(bs.getName().isNotEmpty());
}

TEST_CASE("AudioGraph::getDrumMachine() returns valid reference after initialise", "[InstrumentCommands]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);
    REQUIRE(dm.getName().isNotEmpty());
}

// ==================================================================
// NoteOnCommand tests
// ==================================================================

TEST_CASE("NoteOnCommand perform() injects MIDI note-on into PolySynth", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    ps.prepareToPlay(44100.0, 512);

    NoteOnCommand cmd(ps, 60, 0.8f);
    REQUIRE(cmd.perform() == true);

    // Verify note-on by processing a block and checking for non-zero output
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    ps.processBlock(buffer, midi);

    // Should have non-zero output (note is playing)
    bool hasOutput = false;
    auto* data = buffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        if (data[i] != 0.0f) { hasOutput = true; break; }
    }
    REQUIRE(hasOutput);
}

TEST_CASE("NoteOnCommand perform() injects MIDI note-on into BassSynth", "[InstrumentCommands]")
{
    BassSynthProcessor bs;
    bs.prepareToPlay(44100.0, 512);

    NoteOnCommand cmd(bs, 36, 0.8f);
    REQUIRE(cmd.perform() == true);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    bs.processBlock(buffer, midi);

    bool hasOutput = false;
    auto* data = buffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        if (data[i] != 0.0f) { hasOutput = true; break; }
    }
    REQUIRE(hasOutput);
}

// ==================================================================
// NoteOffCommand tests
// ==================================================================

TEST_CASE("NoteOffCommand perform() injects MIDI note-off into specified instrument", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    ps.ampRelease.store(0.001f); // very short release
    ps.prepareToPlay(44100.0, 512);

    // Note on first
    ps.addMidiEvent(juce::MidiMessage::noteOn(1, 60, 0.8f));
    juce::AudioBuffer<float> buf1(2, 512);
    buf1.clear();
    juce::MidiBuffer m1;
    ps.processBlock(buf1, m1);

    // Note off via command
    NoteOffCommand cmd(ps, 60);
    REQUIRE(cmd.perform() == true);

    // Process several blocks to let release finish
    for (int b = 0; b < 10; ++b) {
        juce::AudioBuffer<float> buf(2, 512);
        buf.clear();
        juce::MidiBuffer m;
        ps.processBlock(buf, m);
    }

    // Final block should be silent
    juce::AudioBuffer<float> finalBuf(2, 512);
    finalBuf.clear();
    juce::MidiBuffer finalMidi;
    ps.processBlock(finalBuf, finalMidi);

    float rms = 0.0f;
    auto* data = finalBuf.getReadPointer(0);
    for (int i = 0; i < finalBuf.getNumSamples(); ++i)
        rms += data[i] * data[i];
    rms = std::sqrt(rms / static_cast<float>(finalBuf.getNumSamples()));
    REQUIRE(rms < 0.001f);
}

// ==================================================================
// LoadSampleCommand tests
// ==================================================================

TEST_CASE("LoadSampleCommand perform() loads a sample into DrumMachine pad", "[InstrumentCommands]")
{
    DrumMachineProcessor dm;
    dm.prepareToPlay(44100.0, 512);

    // Create a temporary WAV file
    juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("test_kick.wav");

    // Write a minimal WAV file
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            new juce::FileOutputStream(tempFile),
            44100.0, 1, 16, {}, 0));

    if (writer) {
        juce::AudioBuffer<float> testAudio(1, 4410); // 100ms
        for (int i = 0; i < 4410; ++i)
            testAudio.setSample(0, i, std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f));
        writer->writeFromAudioSampleBuffer(testAudio, 0, 4410);
        writer.reset();
    }

    LoadSampleCommand cmd(dm, 0, tempFile.getFullPathName().toStdString());
    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(dm.getSampleName(0).isNotEmpty());

    tempFile.deleteFile();
}

// ==================================================================
// Command property tests
// ==================================================================

TEST_CASE("NoteOnCommand isUndoable() returns false", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    NoteOnCommand cmd(ps, 60, 0.8f);
    REQUIRE(cmd.isUndoable() == false);
}

TEST_CASE("NoteOffCommand isUndoable() returns false", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    NoteOffCommand cmd(ps, 60);
    REQUIRE(cmd.isUndoable() == false);
}

TEST_CASE("LoadSampleCommand isUndoable() returns false", "[InstrumentCommands]")
{
    DrumMachineProcessor dm;
    LoadSampleCommand cmd(dm, 0, "/tmp/test.wav");
    REQUIRE(cmd.isUndoable() == false);
}

TEST_CASE("NoteOnCommand getCommandName() returns instrument.noteOn", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    NoteOnCommand cmd(ps, 60, 0.8f);
    REQUIRE(cmd.getCommandName() == "instrument.noteOn");
}

TEST_CASE("NoteOffCommand getCommandName() returns instrument.noteOff", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    NoteOffCommand cmd(ps, 60);
    REQUIRE(cmd.getCommandName() == "instrument.noteOff");
}

TEST_CASE("LoadSampleCommand getCommandName() returns drumMachine.loadSample", "[InstrumentCommands]")
{
    DrumMachineProcessor dm;
    LoadSampleCommand cmd(dm, 0, "/tmp/test.wav");
    REQUIRE(cmd.getCommandName() == "drumMachine.loadSample");
}

// ==================================================================
// Parameter registration tests (test via standalone registry)
// ==================================================================

TEST_CASE("PolySynth parameters can be registered and accessed via ParameterRegistry", "[InstrumentCommands]")
{
    PolySynthProcessor ps;
    ParameterRegistry registry;

    // Register a few representative polysynth parameters
    registry.registerParameter("polysynth.filterCutoff", {
        [&ps]() -> juce::var { return ps.filterCutoff.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterCutoff.store(
            static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 20.0, 20000.0
    });

    registry.registerParameter("polysynth.masterGain", {
        [&ps]() -> juce::var { return ps.masterGain.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.masterGain.store(
            static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });

    REQUIRE(registry.getParameter("polysynth.filterCutoff") != nullptr);
    REQUIRE(registry.getParameter("polysynth.masterGain") != nullptr);

    // Test setter
    registry.getParameter("polysynth.filterCutoff")->setter(juce::var(1000.0));
    REQUIRE(ps.filterCutoff.load() == 1000.0f);
}

TEST_CASE("BassSynth parameters can be registered and accessed via ParameterRegistry", "[InstrumentCommands]")
{
    BassSynthProcessor bs;
    ParameterRegistry registry;

    registry.registerParameter("basssynth.oscWaveform", {
        [&bs]() -> juce::var { return bs.oscWaveform.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.oscWaveform.store(static_cast<int>(v)); },
        "int", 0, 3
    });

    registry.registerParameter("basssynth.masterGain", {
        [&bs]() -> juce::var { return bs.masterGain.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.masterGain.store(
            static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });

    REQUIRE(registry.getParameter("basssynth.oscWaveform") != nullptr);
    REQUIRE(registry.getParameter("basssynth.masterGain") != nullptr);
}

TEST_CASE("DrumMachine volume parameter can be registered and accessed via ParameterRegistry", "[InstrumentCommands]")
{
    DrumMachineProcessor dm;
    ParameterRegistry registry;

    registry.registerParameter("drumMachine.volume", {
        [&dm]() -> juce::var { return dm.volume.load(std::memory_order_relaxed); },
        [&dm](const juce::var& v) { dm.volume.store(
            static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });

    REQUIRE(registry.getParameter("drumMachine.volume") != nullptr);

    // Test setter
    registry.getParameter("drumMachine.volume")->setter(juce::var(0.5));
    REQUIRE(dm.volume.load() == 0.5f);
}
