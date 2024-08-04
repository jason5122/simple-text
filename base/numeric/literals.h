#pragma once

#include <cstddef>
#include <type_traits>

// TODO: See if we should put this under a namespace like "base::literals".
constexpr size_t operator"" _Z(unsigned long long n) {
    return n;
}

namespace {

auto sz = 5_Z;
static_assert(std::is_same<decltype(sz), std::size_t>::value);

}
