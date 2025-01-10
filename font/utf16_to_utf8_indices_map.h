#pragma once

#include "unicode/SkTFitsIn.h"
#include "unicode/unicode.h"

#include <cassert>
#include <vector>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace font {

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#115
class UTF16ToUTF8IndicesMap {
public:
    /** Builds a UTF-16 to UTF-8 indices map; the text is not retained
     * @return true if successful
     */
    bool setUTF8(const char* utf8, size_t size) {
        assert(utf8 != nullptr);

        if (!SkTFitsIn<int32_t>(size)) {
            fmt::println("UTF16ToUTF8IndicesMap: text too long");
            return false;
        }

        int utf16_size = unicode::UTF8ToUTF16(nullptr, 0, utf8, size);
        if (utf16_size < 0) {
            fmt::println("UTF16ToUTF8IndicesMap: Invalid utf8 input");
            return false;
        }

        // utf16_size+1 to also store the size
        utf16_to_utf8_indices = std::vector<size_t>(utf16_size + 1);
        auto utf16 = utf16_to_utf8_indices.begin();
        const char* utf8_begin = utf8;
        const char* utf8_end = utf8 + size;
        while (utf8_begin < utf8_end) {
            *utf16 = utf8_begin - utf8;
            utf16 += unicode::ToUTF16(unicode::NextUTF8(&utf8_begin, utf8_end), nullptr);
        }
        *utf16 = size;

        return true;
    }

    size_t mapIndex(size_t index) const {
        assert(index < utf16_to_utf8_indices.size());
        return utf16_to_utf8_indices[index];
    }

    std::pair<size_t, size_t> mapRange(size_t start, size_t size) const {
        size_t utf8_start = mapIndex(start);
        return {utf8_start, mapIndex(start + size) - utf8_start};
    }

private:
    std::vector<size_t> utf16_to_utf8_indices;
};

}  // namespace font
