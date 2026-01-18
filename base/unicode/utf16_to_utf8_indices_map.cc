#include "base/unicode/unicode.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include <cassert>
#include <utility>

namespace base {

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
    for (size_t i = 0; i < len;) {
        size_t start = i;

        Unichar cp = next_utf8(utf8, i);
        if (cp < 0) {
            map.clear();
            return false;
        }

        int units = codepoint_to_utf16(cp);
        if (units < 0) {
            map.clear();
            return false;
        }

        while (units--) {
            *utf16++ = start;
        }
    }

    return true;
}

}  // namespace base
