#pragma once
#include <juce_core/juce_core.h>
#include <array>

namespace calliope {

// Single-producer, single-consumer lock-free queue using JUCE AbstractFifo.
// T must be trivially copyable. Size is fixed at compile time.
template <typename T, int Capacity>
class LockFreeQueue {
public:
    LockFreeQueue() : fifo_(Capacity) {}

    // Producer: push one item. Returns true if successful, false if full.
    bool push(const T& item) {
        const auto scope = fifo_.write(1);
        if (scope.blockSize1 > 0) {
            buffer_[static_cast<size_t>(scope.startIndex1)] = item;
            return true;
        }
        return false;
    }

    // Consumer: pop one item. Returns true if successful, false if empty.
    bool pop(T& item) {
        const auto scope = fifo_.read(1);
        if (scope.blockSize1 > 0) {
            item = buffer_[static_cast<size_t>(scope.startIndex1)];
            return true;
        }
        return false;
    }

    // Returns the number of items available to read.
    int getNumReady() const { return fifo_.getNumReady(); }

    // Returns the number of free slots available to write.
    int getFreeSpace() const { return fifo_.getFreeSpace(); }

    // Resets the queue to empty state. NOT thread-safe -- call only when no concurrent access.
    void reset() { fifo_.reset(); }

private:
    juce::AbstractFifo fifo_;
    std::array<T, static_cast<size_t>(Capacity)> buffer_;
};

} // namespace calliope
