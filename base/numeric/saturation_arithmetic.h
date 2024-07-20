#pragma once

#include <limits>

namespace base {

#if defined(__has_builtin)
#if __has_builtin(__builtin_add_overflow)

template <class T> constexpr T add_sat(T x, T y) noexcept {
    T result;
    if (!__builtin_add_overflow(x, y, &result)) {
        return result;
    }
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
    T result;
    if (!__builtin_sub_overflow(x, y, &result)) {
        return result;
    }
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

static_assert(add_sat(max_int, 1) == max_int);
static_assert(add_sat(max_size_t, 1UL) == max_size_t);
static_assert(sub_sat(min_int, 1) == min_int);
static_assert(sub_sat(min_size_t, 1UL) == min_size_t);

#endif
#endif

}
