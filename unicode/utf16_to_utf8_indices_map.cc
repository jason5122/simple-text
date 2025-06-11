#include "unicode/fits_in.h"
#include "unicode/unicode.h"
#include "unicode/utf16_to_utf8_indices_map.h"
#include <cassert>

namespace unicode {

bool UTF16ToUTF8IndicesMap::set_utf8(const char* utf8, size_t size) {
    assert(utf8 != nullptr);

    if (!fits_in<int32_t>(size)) {
        return false;
    }

    int utf16_size = unicode::UTF8ToUTF16(nullptr, 0, utf8, size);
    if (utf16_size < 0) {
        return false;
    }

    map = std::vector<size_t>(utf16_size);
    auto utf16 = map.begin();
    const char* utf8_begin = utf8;
    const char* utf8_end = utf8 + size;
    while (utf8_begin < utf8_end) {
        *utf16 = utf8_begin - utf8;
        utf16 += unicode::ToUTF16(unicode::NextUTF8(&utf8_begin, utf8_end), nullptr);
    }

    return true;
}

}  // namespace unicode
