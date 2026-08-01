#pragma once

#include <string>

namespace fuzztest_demo {

// Adds two integers.
inline int Add(int a, int b) { return a + b; }

// Returns `s` with its characters in reverse order.
inline std::string Reverse(const std::string& s) { return std::string(s.rbegin(), s.rend()); }

// Returns true if `s` reads the same forwards and backwards.
inline bool IsPalindrome(const std::string& s) { return s == Reverse(s); }

}  // namespace fuzztest_demo
