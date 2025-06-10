#pragma once

#include "base/numeric/saturation_arithmetic.h"

#include <random>

namespace util {

// Returns a random number in the range [low, high].
inline int random_number(int low, int high) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<> distr{low, high};
    return distr(rng);
}

// Returns a random alphanumeric string of a specified length.
inline std::string random_string(size_t length) {
    static constexpr std::string_view charset = "0123456789"
                                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz";
    auto random_char = []() { return charset.at(random_number(0, charset.length() - 1)); };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, random_char);
    return str;
}

// Like `RandomString()`, but the string is guaranteed to contain a specified number of newlines.
inline std::string random_newline_string(size_t length, size_t newlines) {
    std::string str = random_string(base::sub_sat(length, newlines));
    for (size_t k = 0; k < newlines; ++k) {
        size_t i = util::random_number(0, str.length());
        str.insert(i, "\n");
    }
    return str;
}

inline char random_char() {
    static constexpr auto low = std::numeric_limits<char>::min();
    static constexpr auto high = std::numeric_limits<char>::max();
    return random_number(low, high);
}

}  // namespace util
