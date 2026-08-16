#pragma once

#include "base/check.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include <utility>

namespace base {

// TODO: Implement support for floating points.
template <typename Dst, typename Src>
constexpr Dst checked_cast(Src value) {
    CHECK(std::in_range<Dst>(value));
    return static_cast<Dst>(static_cast<Src>(value));
}

// static_cast<> for a floating-point value to an integer type that clamps to the destination's
// range on overflow and maps NaN to 0 (base::saturated_cast in Chromium, narrowed to the
// float -> integral case we need; use checked_cast for integer -> integer).
template <typename Dst, typename Src>
    requires(std::is_integral_v<Dst> && std::is_floating_point_v<Src>)
constexpr Dst saturated_cast(Src value) {
    if (value != value) {  // NaN
        return 0;
    }
    if (value >= static_cast<Src>(std::numeric_limits<Dst>::max())) {
        return std::numeric_limits<Dst>::max();
    }
    if (value <= static_cast<Src>(std::numeric_limits<Dst>::lowest())) {
        return std::numeric_limits<Dst>::lowest();
    }
    return static_cast<Dst>(value);
}

// Round a floating-point value to an integer with an explicit rounding mode, saturating on
// overflow (base::Clamp{Floor,Ceil,Round} in Chromium). Not constexpr: although we build as C++23,
// this toolchain's libc++ hasn't implemented constexpr std::floor/ceil/round (P0533) yet -- neither
// the std:: functions nor the clang builtins fold in a constant expression. saturated_cast is
// already constexpr, so these can gain it once the library catches up.
template <typename Dst = int, typename Src>
    requires(std::is_integral_v<Dst> && std::is_floating_point_v<Src>)
Dst clamp_floor(Src value) {
    return saturated_cast<Dst>(std::floor(value));
}

template <typename Dst = int, typename Src>
    requires(std::is_integral_v<Dst> && std::is_floating_point_v<Src>)
Dst clamp_ceil(Src value) {
    return saturated_cast<Dst>(std::ceil(value));
}

// Ties away from zero: 0.5 -> 1, -0.5 -> -1.
template <typename Dst = int, typename Src>
    requires(std::is_integral_v<Dst> && std::is_floating_point_v<Src>)
Dst clamp_round(Src value) {
    return saturated_cast<Dst>(std::round(value));
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
