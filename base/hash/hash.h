#pragma once

#include <cstddef>
#include <string>

#include "third_party/hash_maps/rapidhash.h"

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

constexpr uint64_t hash_string(std::string_view str) {
    return rapidhash(str.data(), str.length());
}

constexpr uint64_t hash_string(std::wstring_view str) {
    return rapidhash(str.data(), str.length() * sizeof(wchar_t));
}

}  // namespace base
