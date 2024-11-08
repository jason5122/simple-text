#pragma once

#include "font/types.h"
#include "third_party/xxhash/xxhash.h"
#include <string_view>
#include <unordered_map>

namespace gui {

class LineLayoutCache {
public:
    LineLayoutCache(size_t font_id);

    const font::LineLayout& operator[](std::string_view str8);
    int maxWidth() const;

private:
    size_t font_id;
    std::unordered_map<XXH64_hash_t, font::LineLayout> cache;

    // TODO: Use a data structure (priority queue) for efficient updating.
    int max_width = 0;
};

}  // namespace gui
