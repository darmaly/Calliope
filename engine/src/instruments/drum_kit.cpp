// Built-in drum kit: sixteen synthesised one-shots, so a drum track makes
// sound without the user having to find and load sample files first.

#include "calliope/instruments/drum_machine.h"
#include <cmath>

namespace calliope {

namespace {

constexpr double kKitSampleRate = 44100.0;
constexpr double kTwoPi = 6.283185307179586;

// Presents an in-memory buffer to juce::SamplerSound, which only reads from
// an AudioFormatReader.
class BufferReader : public juce::AudioFormatReader {
public:
    BufferReader(const juce::AudioBuffer<float>& source, double rate)
        : juce::AudioFormatReader(nullptr, "CalliopeBuffer"), buffer_(source)
    {
        sampleRate = rate;
        bitsPerSample = 32;
        lengthInSamples = source.getNumSamples();
        numChannels = static_cast<unsigned int>(source.getNumChannels());
        usesFloatingPointData = true;
    }

    bool readSamples(int* const* destChannels, int numDestChannels, int startOffsetInDestBuffer,
                     juce::int64 startSampleInFile, int numSamples) override
    {
        clearSamplesBeyondAvailableLength(destChannels, numDestChannels, startOffsetInDestBuffer,
                                          startSampleInFile, numSamples, lengthInSamples);
        if (numSamples <= 0)
            return true;

        for (int ch = 0; ch < numDestChannels; ++ch) {
            if (destChannels[ch] == nullptr) continue;
            const int sourceChannel = juce::jmin(ch, buffer_.getNumChannels() - 1);
            auto* dest = reinterpret_cast<float*>(destChannels[ch]) + startOffsetInDestBuffer;
            juce::FloatVectorOperations::copy(
                dest, buffer_.getReadPointer(sourceChannel, static_cast<int>(startSampleInFile)), numSamples);
        }
        return true;
    }

private:
    const juce::AudioBuffer<float>& buffer_;
};

juce::AudioBuffer<float> makeBuffer(double seconds)
{
    juce::AudioBuffer<float> buffer(1, static_cast<int>(seconds * kKitSampleRate));
    buffer.clear();
    return buffer;
}

void normalise(juce::AudioBuffer<float>& buffer, float peak)
{
    const float magnitude = buffer.getMagnitude(0, buffer.getNumSamples());
    if (magnitude > 0.0f)
        buffer.applyGain(peak / magnitude);

    // Short fade so no sample ends on a step.
    const int fade = juce::jmin(256, buffer.getNumSamples());
    buffer.applyGainRamp(buffer.getNumSamples() - fade, fade, 1.0f, 0.0f);
}

// A sine whose pitch falls exponentially from startHz to endHz: the body of a
// kick or a tom.
juce::AudioBuffer<float> makePitchedDrum(double seconds, double startHz, double endHz,
                                         double pitchDecay, double ampDecay, float click)
{
    auto buffer = makeBuffer(seconds);
    auto* data = buffer.getWritePointer(0);
    juce::Random random(1234);
    double phase = 0.0;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const double t = i / kKitSampleRate;
        const double hz = endHz + (startHz - endHz) * std::exp(-t / pitchDecay);
        phase += kTwoPi * hz / kKitSampleRate;
        double sample = std::sin(phase) * std::exp(-t / ampDecay);
        // A few milliseconds of noise give the beater its attack.
        sample += click * (random.nextDouble() * 2.0 - 1.0) * std::exp(-t / 0.003);
        data[i] = static_cast<float>(sample);
    }
    normalise(buffer, 0.95f);
    return buffer;
}

// High-passed noise with an exponential decay: hats and the wires of a snare.
void addNoise(juce::AudioBuffer<float>& buffer, double level, double decay,
              double highPassHz, int seed, double startSeconds = 0.0)
{
    auto* data = buffer.getWritePointer(0);
    juce::Random random(seed);
    const double coefficient = 1.0 - std::exp(-kTwoPi * highPassHz / kKitSampleRate);
    double lowPassed = 0.0;
    const int start = static_cast<int>(startSeconds * kKitSampleRate);
    for (int i = start; i < buffer.getNumSamples(); ++i) {
        const double t = (i - start) / kKitSampleRate;
        const double noise = random.nextDouble() * 2.0 - 1.0;
        lowPassed += coefficient * (noise - lowPassed);
        data[i] += static_cast<float>(level * (noise - lowPassed) * std::exp(-t / decay));
    }
}

void addTone(juce::AudioBuffer<float>& buffer, double hz, double level, double decay)
{
    auto* data = buffer.getWritePointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const double t = i / kKitSampleRate;
        data[i] += static_cast<float>(level * std::sin(kTwoPi * hz * t) * std::exp(-t / decay));
    }
}

juce::AudioBuffer<float> makeSnare(double toneHz, double noiseDecay)
{
    auto buffer = makeBuffer(0.4);
    addTone(buffer, toneHz, 0.6, 0.06);
    addTone(buffer, toneHz * 1.8, 0.3, 0.04);
    addNoise(buffer, 0.8, noiseDecay, 1500.0, 42);
    normalise(buffer, 0.9f);
    return buffer;
}

juce::AudioBuffer<float> makeClap()
{
    auto buffer = makeBuffer(0.45);
    // Several hands landing a few milliseconds apart, then the room.
    addNoise(buffer, 0.7, 0.006, 900.0, 7, 0.000);
    addNoise(buffer, 0.7, 0.006, 900.0, 8, 0.011);
    addNoise(buffer, 0.7, 0.006, 900.0, 9, 0.023);
    addNoise(buffer, 0.9, 0.090, 900.0, 10, 0.032);
    normalise(buffer, 0.9f);
    return buffer;
}

juce::AudioBuffer<float> makeHat(double seconds, double decay)
{
    auto buffer = makeBuffer(seconds);
    addNoise(buffer, 1.0, decay, 7000.0, 99);
    normalise(buffer, 0.7f);
    return buffer;
}

juce::AudioBuffer<float> makeRim()
{
    auto buffer = makeBuffer(0.08);
    addTone(buffer, 820.0, 0.7, 0.012);
    addTone(buffer, 1730.0, 0.5, 0.008);
    addNoise(buffer, 0.3, 0.004, 3000.0, 5);
    normalise(buffer, 0.8f);
    return buffer;
}

// Inharmonic partials plus bright noise: crash and ride.
juce::AudioBuffer<float> makeCymbal(double seconds, double decay, double noiseLevel)
{
    auto buffer = makeBuffer(seconds);
    const double partials[] = { 3170.0, 4230.0, 5410.0, 6790.0, 8130.0, 9870.0 };
    for (double hz : partials)
        addTone(buffer, hz, 0.18, decay * 0.8);
    addNoise(buffer, noiseLevel, decay, 6000.0, 314);
    normalise(buffer, 0.7f);
    return buffer;
}

} // namespace

bool DrumMachineProcessor::loadSampleFromBuffer(int padIndex, const juce::AudioBuffer<float>& buffer,
                                                double sampleRate, const juce::String& name,
                                                double releaseSeconds)
{
    if (padIndex < 0 || padIndex >= kNumPads || buffer.getNumSamples() == 0 || sampleRate <= 0.0)
        return false;

    clearSample(padIndex);

    const int midiNote = kFirstMidiNote + padIndex;
    juce::BigInteger noteRange;
    noteRange.setBit(midiNote);

    BufferReader reader(buffer, sampleRate);
    synth_.addSound(new juce::SamplerSound(
        name, reader, noteRange, midiNote,
        0.0,              // attack
        releaseSeconds,   // release
        30.0));           // maximum length

    padNames_[static_cast<size_t>(padIndex)] = name;
    return true;
}

void DrumMachineProcessor::loadDefaultKit()
{
    struct Pad { const char* name; juce::AudioBuffer<float> audio; };

    // General MIDI percussion layout from note 36 upwards.
    Pad pads[kNumPads] = {
        { "Kick",        makePitchedDrum(0.50, 160.0, 48.0, 0.030, 0.16, 0.35f) },  // 36
        { "Rim",         makeRim() },                                               // 37
        { "Snare",       makeSnare(185.0, 0.11) },                                  // 38
        { "Clap",        makeClap() },                                              // 39
        { "Snare Tight", makeSnare(230.0, 0.06) },                                  // 40
        { "Floor Tom",   makePitchedDrum(0.55, 130.0, 78.0, 0.060, 0.20, 0.15f) },  // 41
        { "Closed Hat",  makeHat(0.12, 0.022) },                                    // 42
        { "Low Tom",     makePitchedDrum(0.50, 160.0, 98.0, 0.060, 0.18, 0.15f) },  // 43
        { "Pedal Hat",   makeHat(0.08, 0.012) },                                    // 44
        { "Mid Tom",     makePitchedDrum(0.45, 200.0, 124.0, 0.055, 0.16, 0.15f) }, // 45
        { "Open Hat",    makeHat(0.60, 0.160) },                                    // 46
        { "High-Mid Tom", makePitchedDrum(0.40, 250.0, 156.0, 0.050, 0.14, 0.15f) },// 47
        { "High Tom",    makePitchedDrum(0.40, 300.0, 190.0, 0.045, 0.13, 0.15f) }, // 48
        { "Crash",       makeCymbal(1.80, 0.50, 0.9) },                             // 49
        { "Top Tom",     makePitchedDrum(0.35, 360.0, 230.0, 0.040, 0.11, 0.15f) }, // 50
        { "Ride",        makeCymbal(1.20, 0.35, 0.35) },                            // 51
    };

    for (int i = 0; i < kNumPads; ++i) {
        // A long release lets every hit ring out however short its note is.
        loadSampleFromBuffer(i, pads[i].audio, kKitSampleRate, pads[i].name, 2.0);
    }
}

} // namespace calliope
