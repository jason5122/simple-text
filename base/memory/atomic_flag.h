#pragma once

#include <atomic>
#include <stdint.h>

namespace base {

// A flag that can safely be set from one thread and read from other threads.
//
// This class IS NOT intended for synchronization between threads.
class AtomicFlag {
public:
    AtomicFlag() {}
    ~AtomicFlag() = default;
    AtomicFlag(const AtomicFlag&) = delete;
    AtomicFlag& operator=(const AtomicFlag&) = delete;

    // Set the flag. Must always be called from the same sequence.
    void Set() { flag_.store(1, std::memory_order_release); }

    // Returns true iff the flag was set. If this returns true, the current thread
    // is guaranteed to be synchronized with all memory operations on the sequence
    // which invoked Set() up until at least the first call to Set() on it.
    bool IsSet() const {
        // Inline here: this has a measurable performance impact on base::WeakPtr.
        return flag_.load(std::memory_order_acquire) != 0;
    }

private:
    std::atomic<uint_fast8_t> flag_{0};
};

}  // namespace base
