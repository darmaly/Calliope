#include "calliope/insert_chain.h"
#include <algorithm>

namespace calliope {

InsertChain::InsertChain()
    : chainA_(std::make_unique<ChainState>()),
      chainB_(std::make_unique<ChainState>())
{
    activeChain_.store(chainA_.get(), std::memory_order_release);
    activeIsA_ = true;
}

InsertChain::~InsertChain() = default;

void InsertChain::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    auto* chain = activeChain_.load(std::memory_order_acquire);
    if (!chain) return;

    for (size_t i = 0; i < chain->effects.size(); ++i) {
        if (!chain->bypassed[i] && chain->effects[i]) {
            chain->effects[i]->processBlock(buffer, midi);
        }
    }
}

void InsertChain::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate_ = sampleRate;
    currentBlockSize_ = samplesPerBlock;

    auto* chain = activeChain_.load(std::memory_order_acquire);
    if (!chain) return;

    for (auto& effect : chain->effects) {
        if (effect)
            prepareEffect(effect.get());
    }
}

int InsertChain::insertEffect(std::unique_ptr<juce::AudioProcessor> effect, int position)
{
    auto* active = activeChain_.load(std::memory_order_acquire);
    auto* inactive = getInactiveChain();

    // Clear inactive and rebuild from active
    inactive->effects.clear();
    inactive->bypassed.clear();

    if (active) {
        for (size_t i = 0; i < active->effects.size(); ++i) {
            inactive->effects.push_back(std::move(active->effects[i]));
            inactive->bypassed.push_back(active->bypassed[i]);
        }
    }

    // Prepare the new effect before inserting
    prepareEffect(effect.get());

    int insertPos;
    if (position < 0 || position >= static_cast<int>(inactive->effects.size())) {
        insertPos = static_cast<int>(inactive->effects.size());
        inactive->effects.push_back(std::move(effect));
        inactive->bypassed.push_back(false);
    } else {
        insertPos = position;
        inactive->effects.insert(inactive->effects.begin() + position, std::move(effect));
        inactive->bypassed.insert(inactive->bypassed.begin() + position, false);
    }

    swapChains();
    return insertPos;
}

std::unique_ptr<juce::AudioProcessor> InsertChain::removeEffect(int position)
{
    auto* active = activeChain_.load(std::memory_order_acquire);
    if (!active || position < 0 || position >= static_cast<int>(active->effects.size()))
        return nullptr;

    auto* inactive = getInactiveChain();

    // Rebuild without the removed effect
    inactive->effects.clear();
    inactive->bypassed.clear();

    std::unique_ptr<juce::AudioProcessor> removed;
    for (size_t i = 0; i < active->effects.size(); ++i) {
        if (static_cast<int>(i) == position) {
            removed = std::move(active->effects[i]);
        } else {
            inactive->effects.push_back(std::move(active->effects[i]));
            inactive->bypassed.push_back(active->bypassed[i]);
        }
    }

    swapChains();
    return removed;
}

void InsertChain::moveEffect(int fromPosition, int toPosition)
{
    auto* active = activeChain_.load(std::memory_order_acquire);
    if (!active) return;

    int size = static_cast<int>(active->effects.size());
    if (fromPosition < 0 || fromPosition >= size || toPosition < 0 || toPosition >= size)
        return;

    auto* inactive = getInactiveChain();

    // Copy all effects to inactive
    inactive->effects.clear();
    inactive->bypassed.clear();
    for (size_t i = 0; i < active->effects.size(); ++i) {
        inactive->effects.push_back(std::move(active->effects[i]));
        inactive->bypassed.push_back(active->bypassed[i]);
    }

    // Perform the move
    auto movedEffect = std::move(inactive->effects[fromPosition]);
    bool movedBypass = inactive->bypassed[fromPosition];

    inactive->effects.erase(inactive->effects.begin() + fromPosition);
    inactive->bypassed.erase(inactive->bypassed.begin() + fromPosition);

    inactive->effects.insert(inactive->effects.begin() + toPosition, std::move(movedEffect));
    inactive->bypassed.insert(inactive->bypassed.begin() + toPosition, movedBypass);

    swapChains();
}

void InsertChain::setBypass(int position, bool bypassed)
{
    auto* active = activeChain_.load(std::memory_order_acquire);
    if (!active || position < 0 || position >= static_cast<int>(active->effects.size()))
        return;

    auto* inactive = getInactiveChain();

    // Copy to inactive
    inactive->effects.clear();
    inactive->bypassed.clear();
    for (size_t i = 0; i < active->effects.size(); ++i) {
        inactive->effects.push_back(std::move(active->effects[i]));
        inactive->bypassed.push_back(active->bypassed[i]);
    }

    inactive->bypassed[position] = bypassed;

    swapChains();
}

bool InsertChain::isBypassed(int position) const
{
    auto* chain = activeChain_.load(std::memory_order_acquire);
    if (!chain || position < 0 || position >= static_cast<int>(chain->bypassed.size()))
        return false;
    return chain->bypassed[position];
}

int InsertChain::getNumEffects() const
{
    auto* chain = activeChain_.load(std::memory_order_acquire);
    if (!chain) return 0;
    return static_cast<int>(chain->effects.size());
}

juce::AudioProcessor* InsertChain::getEffect(int position) const
{
    auto* chain = activeChain_.load(std::memory_order_acquire);
    if (!chain || position < 0 || position >= static_cast<int>(chain->effects.size()))
        return nullptr;
    return chain->effects[position].get();
}

InsertChain::ChainState* InsertChain::getInactiveChain()
{
    return activeIsA_ ? chainB_.get() : chainA_.get();
}

void InsertChain::swapChains()
{
    auto* inactive = getInactiveChain();
    activeChain_.store(inactive, std::memory_order_release);
    activeIsA_ = !activeIsA_;
}

void InsertChain::prepareEffect(juce::AudioProcessor* effect)
{
    if (effect) {
        effect->setPlayConfigDetails(2, 2, currentSampleRate_, currentBlockSize_);
        effect->prepareToPlay(currentSampleRate_, currentBlockSize_);
    }
}

} // namespace calliope
