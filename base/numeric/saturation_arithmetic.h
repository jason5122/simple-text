#pragma once

#include "base/compiler_specific.h"
#include "base/numeric/safe_conversions.h"
#include <concepts>
#include <limits>
#include <type_traits>

namespace base {

template <class T>
constexpr T add_sat(T x, T y) noexcept {
#if HAS_BUILTIN(__builtin_add_overflow)
    if (T result; !__builtin_add_overflow(x, y, &result)) {
        return result;
    }
#else
    if ((x > 0 && y <= std::numeric_limits<T>::max() - x) ||
        (x <= 0 && y >= std::numeric_limits<T>::min() - x)) {
        return x + y;
    }
#endif
    // Handle overflow.
    if constexpr (std::is_unsigned_v<T>) {
        return std::numeric_limits<T>::max();
    } else {
        // Signed addition overflow.
        if (x > 0) {
            // Overflows if (x > 0 && y > 0).
            return std::numeric_limits<T>::max();
        } else {
            // Overflows if  (x < 0 && y < 0).
            return std::numeric_limits<T>::min();
        }
    }
}

template <class T>
constexpr T sub_sat(T x, T y) noexcept {
#if HAS_BUILTIN(__builtin_add_overflow)
    if (T result; !__builtin_sub_overflow(x, y, &result)) {
        return result;
    }
#else
    if ((y > 0 && x >= std::numeric_limits<T>::min() + y) ||
        (y <= 0 && x <= std::numeric_limits<T>::max() + y)) {
        return x - y;
    }
#endif
    // Handle overflow.
    if constexpr (std::is_unsigned_v<T>) {
        return std::numeric_limits<T>::min();
    } else {
        // Signed subtraction overflow.
        if (x > 0) {
            // Overflows if (x >= 0 && y < 0).
            return std::numeric_limits<T>::max();
        } else {
            // Overflows if (x < 0 && y > 0).
            return std::numeric_limits<T>::min();
        }
    }
}

template <typename T>
    requires(std::integral<T>)
constexpr T abs_sat(T value) {
    // The calculation below is a static identity for unsigned types, but for
    // signed integer types it provides a non-branching, saturated absolute value.
    // This works because safe_unsigned_abs() returns an unsigned type, which can
    // represent the absolute value of all negative numbers of an equal-width
    // integer type. The call to is_value_negative() then detects overflow in the
    // special case of numeric_limits<T>::min(), by evaluating the bit pattern as
    // a signed integer value. If it is the overflow case, we end up subtracting
    // one from the unsigned result, thus saturating to numeric_limits<T>::max().
    return static_cast<T>(safe_unsigned_abs(value) -
                          is_value_negative<T>(static_cast<T>(safe_unsigned_abs(value))));
}

template <typename T>
    requires(std::floating_point<T>)
constexpr T abs_sat(T value) {
    return value < 0 ? -value : value;
}

}  // namespace base
