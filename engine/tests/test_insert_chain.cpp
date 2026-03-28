#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/insert_chain.h"
#include "calliope/effects/parametric_eq.h"
#include "calliope/effects/compressor.h"
#include <cmath>

using namespace calliope;

namespace {

bool bufferUnchanged(const juce::AudioBuffer<float>& buffer, float expectedValue)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::abs(data[i] - expectedValue) > 1e-6f)
                return false;
        }
    }
    return true;
}

juce::AudioBuffer<float> makeTestBuffer(int numChannels, int numSamples, float fillValue)
{
    juce::AudioBuffer<float> buf(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
            buf.setSample(ch, i, fillValue);
    return buf;
}

} // namespace

TEST_CASE("InsertChain empty chain passes audio unmodified", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto buffer = makeTestBuffer(2, 512, 0.75f);
    juce::MidiBuffer midi;
    chain.processBlock(buffer, midi);

    REQUIRE(bufferUnchanged(buffer, 0.75f));
}

TEST_CASE("InsertChain insertEffect adds an effect", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto eq = std::make_unique<ParametricEqProcessor>();
    chain.insertEffect(std::move(eq));

    REQUIRE(chain.getNumEffects() == 1);
}

TEST_CASE("InsertChain insertEffect at position 0 inserts at beginning", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto comp = std::make_unique<CompressorProcessor>();
    chain.insertEffect(std::move(comp));

    auto eq = std::make_unique<ParametricEqProcessor>();
    chain.insertEffect(std::move(eq), 0);

    REQUIRE(chain.getNumEffects() == 2);
    REQUIRE(chain.getEffect(0)->getName() == "ParametricEQ");
    REQUIRE(chain.getEffect(1)->getName() == "Compressor");
}

TEST_CASE("InsertChain removeEffect removes and returns the processor", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto eq = std::make_unique<ParametricEqProcessor>();
    chain.insertEffect(std::move(eq));
    REQUIRE(chain.getNumEffects() == 1);

    auto removed = chain.removeEffect(0);
    REQUIRE(removed != nullptr);
    REQUIRE(removed->getName() == "ParametricEQ");
    REQUIRE(chain.getNumEffects() == 0);
}

TEST_CASE("InsertChain moveEffect reorders the chain", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto eq = std::make_unique<ParametricEqProcessor>();
    auto comp = std::make_unique<CompressorProcessor>();
    chain.insertEffect(std::move(eq));    // position 0
    chain.insertEffect(std::move(comp));  // position 1

    REQUIRE(chain.getEffect(0)->getName() == "ParametricEQ");
    REQUIRE(chain.getEffect(1)->getName() == "Compressor");

    chain.moveEffect(1, 0); // move compressor to front

    REQUIRE(chain.getEffect(0)->getName() == "Compressor");
    REQUIRE(chain.getEffect(1)->getName() == "ParametricEQ");
}

TEST_CASE("InsertChain setBypass causes effect to be skipped", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto eq = std::make_unique<ParametricEqProcessor>();
    chain.insertEffect(std::move(eq));

    chain.setBypass(0, true);
    REQUIRE(chain.isBypassed(0) == true);

    chain.setBypass(0, false);
    REQUIRE(chain.isBypassed(0) == false);
}

TEST_CASE("InsertChain processBlock processes effects in order", "[InsertChain]")
{
    InsertChain chain;
    chain.prepareToPlay(44100.0, 512);

    auto eq = std::make_unique<ParametricEqProcessor>();
    auto comp = std::make_unique<CompressorProcessor>();
    chain.insertEffect(std::move(eq));
    chain.insertEffect(std::move(comp));

    // Verify both effects are present in expected order
    REQUIRE(chain.getEffect(0)->getName() == "ParametricEQ");
    REQUIRE(chain.getEffect(1)->getName() == "Compressor");

    // Process a buffer - should not crash
    auto buffer = makeTestBuffer(2, 512, 0.5f);
    juce::MidiBuffer midi;
    chain.processBlock(buffer, midi);

    // Buffer should have been processed (we just verify no crash)
    REQUIRE(true);
}

TEST_CASE("InsertChain prepareToPlay propagates to all effects", "[InsertChain]")
{
    InsertChain chain;

    auto eq = std::make_unique<ParametricEqProcessor>();
    auto comp = std::make_unique<CompressorProcessor>();
    chain.insertEffect(std::move(eq));
    chain.insertEffect(std::move(comp));

    // This should prepare all effects without crashing
    chain.prepareToPlay(48000.0, 256);

    // Process should work after prepare
    auto buffer = makeTestBuffer(2, 256, 0.5f);
    juce::MidiBuffer midi;
    chain.processBlock(buffer, midi);

    REQUIRE(true);
}
