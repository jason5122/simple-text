#include "utf8_string.h"

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace base {

Utf8String::Utf8String(const std::string& str8) : str8{str8} {
    size_t offset;
    for (size_t total_offset = 0; total_offset < str8.size(); total_offset += offset) {
        offset = grapheme_next_character_break_utf8(&str8[0] + total_offset, SIZE_MAX);
        utf8_chars.emplace_back(Utf8Char{
            .str = std::string_view(this->str8).substr(total_offset, offset),
            .size = offset,
            .offset = total_offset,
        });
    }
}

const std::vector<Utf8String::Utf8Char>& Utf8String::getChars() const {
    return utf8_chars;
}

}
