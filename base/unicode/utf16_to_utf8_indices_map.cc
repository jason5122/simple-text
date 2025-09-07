#include "base/unicode/unicode.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include <cassert>

namespace base {

namespace {

// Returns the number of resulting UTF-16 code units needed to convert the UTF-8 sequence.
// If there is an error, -1 is returned.
int utf8_to_utf16_length(std::string_view utf8) {
    int len = 0;
    for (size_t i = 0; i < utf8.length();) {
        Unichar cp = next_utf8(utf8, i);
        if (cp < 0) return -1;

        int count = codepoint_to_utf16(cp);
        if (count < 0) return -1;

        len += count;
    }
    return len;
}

}  // namespace

bool UTF16ToUTF8IndicesMap::set_utf8(std::string_view utf8) {
    size_t len = utf8.length();

    if (!std::in_range<int32_t>(len)) {
        return false;
    }

    int utf16_size = utf8_to_utf16_length(utf8);
    if (utf16_size < 0) {
        return false;
    }

    map = std::vector<size_t>(utf16_size);
    auto utf16 = map.begin();
    size_t i = 0;
    while (i < len) {
        *utf16 = i;
        utf16 += codepoint_to_utf16(next_utf8(utf8, i));
    }

    return true;
}

}  // namespace base
