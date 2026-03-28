#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <vector>

namespace calliope {

// InsertChain manages an ordered list of effect processors that process
// audio serially. Uses a double-buffer pattern for real-time safe chain
// modification: the message thread builds a new chain state in the inactive
// buffer, then atomically swaps the pointer for the audio thread.
class InsertChain {
public:
    InsertChain();
    ~InsertChain();

    // Audio thread -- must be lock-free
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // Propagates prepareToPlay to all effects in the active chain
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Message thread -- may allocate
    // Returns the slot index where the effect was inserted.
    // position = -1 means append to end.
    int insertEffect(std::unique_ptr<juce::AudioProcessor> effect, int position = -1);

    // Removes and returns the processor at the given position.
    std::unique_ptr<juce::AudioProcessor> removeEffect(int position);

    // Reorders: moves effect from 'fromPosition' to 'toPosition'.
    void moveEffect(int fromPosition, int toPosition);

    // Per-effect bypass
    void setBypass(int position, bool bypassed);
    bool isBypassed(int position) const;

    int getNumEffects() const;
    juce::AudioProcessor* getEffect(int position) const;

private:
    struct ChainState {
        std::vector<std::unique_ptr<juce::AudioProcessor>> effects;
        std::vector<bool> bypassed;
    };

    // Audio thread reads activeChain_ via atomic pointer
    std::atomic<ChainState*> activeChain_{nullptr};

    // Message thread owns both buffers and swaps between them
    std::unique_ptr<ChainState> chainA_;
    std::unique_ptr<ChainState> chainB_;

    // Identifies which buffer is currently active (A=true, B=false)
    bool activeIsA_ = true;

    double currentSampleRate_ = 44100.0;
    int currentBlockSize_ = 512;

    // Get the inactive chain state for building the next version
    ChainState* getInactiveChain();

    // Copy effects from source to dest (transfers ownership via move)
    void copyChainTo(ChainState* source, ChainState* dest);

    // Swap the active pointer
    void swapChains();

    // Prepare a single effect
    void prepareEffect(juce::AudioProcessor* effect);
};

} // namespace calliope
