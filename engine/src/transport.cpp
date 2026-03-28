#include "calliope/transport.h"

static_assert(std::atomic<double>::is_always_lock_free,
              "atomic<double> must be lock-free for real-time audio");

namespace calliope {

Transport::Transport() = default;

juce::Optional<juce::AudioPlayHead::PositionInfo> Transport::getPosition() const
{
    PositionInfo info;

    const auto currentBpm = bpm_.load(std::memory_order_relaxed);
    const auto currentSampleRate = sampleRate_.load(std::memory_order_relaxed);
    const auto currentSamplePos = samplePosition_.load(std::memory_order_relaxed);
    const auto currentState = state_.load(std::memory_order_relaxed);

    // Calculate PPQ position: (samples / sampleRate) * (bpm / 60.0)
    const double timeInSeconds = static_cast<double>(currentSamplePos) / currentSampleRate;
    const double ppq = timeInSeconds * (currentBpm / 60.0);

    info.setBpm(currentBpm);
    info.setTimeSignature(juce::AudioPlayHead::TimeSignature{
        timeSigNumerator_.load(std::memory_order_relaxed),
        timeSigDenominator_.load(std::memory_order_relaxed)
    });
    info.setIsPlaying(currentState == TransportState::Playing);
    info.setPpqPosition(ppq);
    info.setTimeInSamples(currentSamplePos);

    const bool loopEnabled = looping_.load(std::memory_order_relaxed);
    info.setIsLooping(loopEnabled);

    if (loopEnabled)
    {
        const auto loopStart = loopStartBeat_.load(std::memory_order_relaxed);
        const auto loopEnd = loopEndBeat_.load(std::memory_order_relaxed);
        info.setLoopPoints(juce::AudioPlayHead::LoopPoints{loopStart, loopEnd});
    }

    return info;
}

void Transport::advancePosition(int numSamples)
{
    if (state_.load(std::memory_order_relaxed) != TransportState::Playing)
        return;

    auto pos = samplePosition_.load(std::memory_order_relaxed);
    pos += numSamples;

    if (looping_.load(std::memory_order_relaxed))
    {
        const auto currentBpm = bpm_.load(std::memory_order_relaxed);
        const auto currentSampleRate = sampleRate_.load(std::memory_order_relaxed);
        const auto beatsPerSecond = currentBpm / 60.0;

        const auto loopEndBeat = loopEndBeat_.load(std::memory_order_relaxed);
        const auto loopStartBeat = loopStartBeat_.load(std::memory_order_relaxed);

        const auto loopEndSamples = static_cast<int64_t>((loopEndBeat / beatsPerSecond) * currentSampleRate);
        const auto loopStartSamples = static_cast<int64_t>((loopStartBeat / beatsPerSecond) * currentSampleRate);

        if (pos >= loopEndSamples)
        {
            const auto loopLength = loopEndSamples - loopStartSamples;
            if (loopLength > 0)
                pos = loopStartSamples + ((pos - loopStartSamples) % loopLength);
            else
                pos = loopStartSamples;
        }
    }

    samplePosition_.store(pos, std::memory_order_relaxed);
}

void Transport::play()
{
    state_.store(TransportState::Playing, std::memory_order_relaxed);
}

void Transport::stop()
{
    state_.store(TransportState::Stopped, std::memory_order_relaxed);
    samplePosition_.store(0, std::memory_order_relaxed);
}

void Transport::pause()
{
    state_.store(TransportState::Paused, std::memory_order_relaxed);
}

void Transport::setBpm(double newBpm)
{
    bpm_.store(newBpm, std::memory_order_relaxed);
}

void Transport::setTimeSignature(int numerator, int denominator)
{
    timeSigNumerator_.store(numerator, std::memory_order_relaxed);
    timeSigDenominator_.store(denominator, std::memory_order_relaxed);
}

void Transport::setLoopRegion(double startBeat, double endBeat, bool enabled)
{
    loopStartBeat_.store(startBeat, std::memory_order_relaxed);
    loopEndBeat_.store(endBeat, std::memory_order_relaxed);
    looping_.store(enabled, std::memory_order_relaxed);
}

void Transport::setSampleRate(double newSampleRate)
{
    sampleRate_.store(newSampleRate, std::memory_order_relaxed);
}

TransportState Transport::getState() const
{
    return state_.load(std::memory_order_relaxed);
}

double Transport::getBpm() const
{
    return bpm_.load(std::memory_order_relaxed);
}

int Transport::getTimeSignatureNumerator() const
{
    return timeSigNumerator_.load(std::memory_order_relaxed);
}

int Transport::getTimeSignatureDenominator() const
{
    return timeSigDenominator_.load(std::memory_order_relaxed);
}

int64_t Transport::getSamplePosition() const
{
    return samplePosition_.load(std::memory_order_relaxed);
}

double Transport::getPpqPosition() const
{
    const auto currentBpm = bpm_.load(std::memory_order_relaxed);
    const auto currentSampleRate = sampleRate_.load(std::memory_order_relaxed);
    const auto currentSamplePos = samplePosition_.load(std::memory_order_relaxed);

    const double timeInSeconds = static_cast<double>(currentSamplePos) / currentSampleRate;
    return timeInSeconds * (currentBpm / 60.0);
}

bool Transport::isLooping() const
{
    return looping_.load(std::memory_order_relaxed);
}

double Transport::getLoopStartBeat() const
{
    return loopStartBeat_.load(std::memory_order_relaxed);
}

double Transport::getLoopEndBeat() const
{
    return loopEndBeat_.load(std::memory_order_relaxed);
}

} // namespace calliope
