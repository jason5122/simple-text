#pragma once

#include <string_view>
#include <vector>

// References:
// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#115

namespace unicode {

// Builds a UTF-16 to UTF-8 indices map. The text is not retained.
class UTF16ToUTF8IndicesMap {
public:
    bool set_utf8(std::string_view str8);
    constexpr size_t map_index(size_t index) const { return map[index]; }

private:
    std::vector<size_t> map;
};

}  // namespace unicode
