#pragma once

#include <string>

constexpr std::string EscapeSpecialChars(const std::string& str) {
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
