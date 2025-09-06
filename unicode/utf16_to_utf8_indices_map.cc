#include "unicode/unicode.h"
#include "unicode/utf16_to_utf8_indices_map.h"
#include <cassert>

namespace unicode {

bool UTF16ToUTF8IndicesMap::set_utf8(std::string_view str8) {
    // TODO: Consider refactoring to not use `const char*`.
    const char* utf8 = str8.data();
    size_t size = str8.length();

    if (!std::in_range<int32_t>(size)) {
        return false;
    }

    int utf16_size = utf8_to_utf16(nullptr, 0, utf8, size);
    if (utf16_size < 0) {
        return false;
    }

    map = std::vector<size_t>(utf16_size);
    auto utf16 = map.begin();
    const char* utf8_begin = utf8;
    const char* utf8_end = utf8 + size;
    while (utf8_begin < utf8_end) {
        *utf16 = utf8_begin - utf8;
        utf16 += to_utf16(next_utf8(&utf8_begin, utf8_end), nullptr);
    }

    return true;
}

}  // namespace unicode
