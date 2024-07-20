#pragma once

#include "base/compiler_specific.h"

#include <limits>

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

static_assert(std::is_unsigned<int>::value == false);
static_assert(std::is_unsigned<size_t>::value == true);

constexpr int min_int = std::numeric_limits<int>::min();
constexpr int max_int = std::numeric_limits<int>::max();
constexpr size_t min_size_t = std::numeric_limits<size_t>::min();
constexpr size_t max_size_t = std::numeric_limits<size_t>::max();

// No overflow (simple tests).
static_assert(add_sat(10, 5) == 15);
static_assert(add_sat(-10, 5) == -5);
static_assert(add_sat(10UL, 5UL) == 15UL);
// No overflow (boundary tests).
static_assert(add_sat(max_int - 5, 5) == max_int);
static_assert(add_sat(min_int + 5, -5) == min_int);
static_assert(add_sat(max_size_t - 5UL, 5UL) == max_size_t);
// Overflow.
static_assert(add_sat(max_int, 1) == max_int);
static_assert(add_sat(max_size_t, 1UL) == max_size_t);

// No overflow (simple tests).
static_assert(sub_sat(10, 5) == 5);
static_assert(sub_sat(-10, 5) == -15);
static_assert(sub_sat(10UL, 5UL) == 5UL);
// No overflow (boundary tests).
static_assert(sub_sat(min_int + 5, 5) == min_int);
static_assert(sub_sat(max_int - 5, -5) == max_int);
static_assert(sub_sat(min_size_t + 5UL, 5UL) == min_size_t);
// Overflow.
static_assert(sub_sat(min_int, 1) == min_int);
static_assert(sub_sat(min_size_t, 1UL) == min_size_t);

}
