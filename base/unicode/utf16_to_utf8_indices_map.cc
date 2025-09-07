#include "base/unicode/unicode.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include <cassert>

namespace base {

bool UTF16ToUTF8IndicesMap::set_utf8(std::string_view str8) {
    size_t len = str8.length();

    if (!std::in_range<int32_t>(len)) {
        return false;
    }

    int utf16_size = utf8_to_utf16_length(str8);
    if (utf16_size < 0) {
        return false;
    }

    map = std::vector<size_t>(utf16_size);
    auto utf16 = map.begin();
    size_t i = 0;
    while (i < len) {
        *utf16 = i;
        utf16 += to_utf16(next_utf8(str8, i), nullptr);
    }

    return true;
}

}  // namespace base
