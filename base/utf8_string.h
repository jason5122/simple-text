#pragma once

#include <string>
#include <vector>

namespace base {

class Utf8String {
public:
    struct Utf8Char {
        std::string_view str;
        size_t size;
        size_t offset;
    };

    Utf8String(const std::string& str8);
    const std::vector<Utf8Char>& getChars() const;

private:
    std::string str8;
    std::vector<Utf8Char> utf8_chars;
};

}
