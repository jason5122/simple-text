#pragma once

#include <vector>

// References:
// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#115

namespace unicode {

// Builds a UTF-16 to UTF-8 indices map. The text is not retained.
class UTF16ToUTF8IndicesMap {
public:
    bool set_utf8(const char* utf8, size_t size);

    constexpr size_t map_index(size_t index) const {
        return map[index];
    }

    constexpr std::pair<size_t, size_t> map_range(size_t start, size_t size) const {
        size_t utf8_start = map_index(start);
        return {utf8_start, map_index(start + size) - utf8_start};
    }

private:
    std::vector<size_t> map;
};

}  // namespace unicode
