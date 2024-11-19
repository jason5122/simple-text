#pragma once

#include <cassert>
#include <cstddef>

namespace base {

constexpr size_t inc_wrap(size_t i, size_t size) noexcept {
    assert(i < size);

    if (i + 1 == size) {
        return 0;
    } else {
        return i + 1;
    }
}

constexpr size_t dec_wrap(size_t i, size_t size) noexcept {
    assert(i < size);

    if (i == 0) {
        return size - 1;
    } else {
        return i - 1;
    }
}

}  // namespace base
