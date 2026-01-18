#pragma once

#include "base/check.h"
#include <cstdlib>
#include <type_traits>
#include <utility>

namespace base {

// TODO: Implement support for floating points.
template <typename Dst, typename Src>
constexpr Dst checked_cast(Src value) {
    CHECK(std::in_range<Dst>(value));
    return static_cast<Dst>(static_cast<Src>(value));
}

// Determines if a numeric value is negative without throwing compiler.
template <typename T>
    requires(std::is_arithmetic_v<T>)
constexpr bool is_value_negative(T value) {
    if constexpr (std::is_signed_v<T>) {
        return value < 0;
    } else {
        return false;
    }
}

// This performs a safe, absolute value via unsigned overflow.
template <typename T>
    requires(std::is_integral_v<T>)
constexpr auto safe_unsigned_abs(T value) {
    using UnsignedT = std::make_unsigned_t<T>;
    return is_value_negative(value) ? static_cast<UnsignedT>(0u - static_cast<UnsignedT>(value))
                                    : static_cast<UnsignedT>(value);
}

}  // namespace base
