#include <catch2/catch_test_macros.hpp>
#include "calliope/metronome.h"

using namespace calliope;

TEST_CASE("MetronomeProcessor starts enabled with volume 0.7", "[Metronome]")
{
    MetronomeProcessor metro;
    REQUIRE(metro.enabled.load() == true);
    REQUIRE(metro.volume.load() == 0.7f);
}

TEST_CASE("MetronomeProcessor clears buffer when disabled", "[Metronome]")
{
    MetronomeProcessor metro;
    metro.enabled.store(false);
    metro.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    // Fill with non-zero data
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 1.0f);

    juce::MidiBuffer midi;
    metro.processBlock(buffer, midi);

    // Verify buffer is all zeros
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE(buffer.getSample(ch, i) == 0.0f);
}

TEST_CASE("MetronomeProcessor clears buffer when no playhead", "[Metronome]")
{
    MetronomeProcessor metro;
    metro.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 1.0f);

    juce::MidiBuffer midi;
    metro.processBlock(buffer, midi);

    // No playhead set -> buffer should be cleared
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE(buffer.getSample(ch, i) == 0.0f);
}

TEST_CASE("MetronomeProcessor getName returns Metronome", "[Metronome]")
{
    MetronomeProcessor metro;
    REQUIRE(metro.getName() == "Metronome");
}

TEST_CASE("MetronomeProcessor produces stereo output", "[Metronome]")
{
    MetronomeProcessor metro;
    REQUIRE(metro.acceptsMidi() == false);
    REQUIRE(metro.producesMidi() == false);
    // Verify output bus has 2 channels (stereo)
    REQUIRE(metro.getTotalNumOutputChannels() == 2);
    // Verify no input bus
    REQUIRE(metro.getTotalNumInputChannels() == 0);
}
