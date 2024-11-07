#pragma once

#include <string>

namespace util {

constexpr std::string EscapeSpecialChars(std::string_view str) {
    std::string result;
    for (auto c : str) {
        switch (c) {
        case '\n':
            result += "\\n";
            break;

        case '\t':
            result += "\\t";
            break;

        case '\r':
            result += "\\r";
            break;

        default:
            result += c;
            break;
        }
    }
    return result;
}

}  // namespace util
