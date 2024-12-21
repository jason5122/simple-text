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

        auto utf16Size = unicode::UTF8ToUTF16(nullptr, 0, utf8, size);
        if (utf16Size < 0) {
            fmt::println("UTF16ToUTF8IndicesMap: Invalid utf8 input");
            return false;
        }

        // utf16Size+1 to also store the size
        fUtf16ToUtf8Indices = std::vector<size_t>(utf16Size + 1);
        auto utf16 = fUtf16ToUtf8Indices.begin();
        auto utf8Begin = utf8, utf8End = utf8 + size;
        while (utf8Begin < utf8End) {
            *utf16 = utf8Begin - utf8;
            utf16 += unicode::ToUTF16(unicode::NextUTF8(&utf8Begin, utf8End), nullptr);
        }
        *utf16 = size;

        return true;
    }

    size_t mapIndex(size_t index) const {
        assert(index < fUtf16ToUtf8Indices.size());
        return fUtf16ToUtf8Indices[index];
    }

    std::pair<size_t, size_t> mapRange(size_t start, size_t size) const {
        auto utf8Start = mapIndex(start);
        return {utf8Start, mapIndex(start + size) - utf8Start};
    }

private:
    std::vector<size_t> fUtf16ToUtf8Indices;
};

}
