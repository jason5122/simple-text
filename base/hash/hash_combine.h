#pragma once

#include <cstddef>

namespace base {

// https://stackoverflow.com/a/27952689/14698275
// https://nnethercote.github.io/2021/12/08/a-brutally-effective-hash-function-in-rust.html
constexpr size_t hash_combine(size_t lhs, size_t rhs) {
    if constexpr (sizeof(size_t) >= 8) {
        lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
    } else {
        lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    }
    return lhs;
}

}  // namespace base
