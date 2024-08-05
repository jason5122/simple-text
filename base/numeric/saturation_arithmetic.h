#pragma once

#include "base/compiler_specific.h"
#include <limits>
#include <type_traits>

namespace base {

template <class T> constexpr T add_sat(T x, T y) noexcept {
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
    if constexpr (std::is_unsigned<T>::value) {
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

template <class T> constexpr T sub_sat(T x, T y) noexcept {
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
    if constexpr (std::is_unsigned<T>::value) {
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

}
