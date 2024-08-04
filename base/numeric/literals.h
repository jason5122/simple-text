#pragma once

#include <cstddef>
#include <type_traits>

namespace base::literals {

constexpr size_t operator"" _z(unsigned long long n) {
    return n;
}

}

namespace {

using namespace base::literals;

auto sz = 5_z;
static_assert(std::is_same<decltype(sz), std::size_t>::value);

}
