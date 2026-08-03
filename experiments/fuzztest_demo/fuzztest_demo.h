#pragma once

#include <string>

namespace fuzztest_demo {

inline int add(int a, int b) { return a + b; }
inline std::string reverse(const std::string& s) { return std::string(s.rbegin(), s.rend()); }
inline bool is_palindrome(const std::string& s) { return s == reverse(s); }
inline int average(int a, int b) { return (a + b) / 2; }

inline std::string encode_newlines(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\n') {
            out += '\\';
            out += 'n';
        } else {
            out += c;
        }
    }
    return out;
}
inline std::string decode_newlines(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') {
            out += '\n';
            ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

}  // namespace fuzztest_demo
