#pragma once

#include "font/types.h"
#include "third_party/xxhash/xxhash.h"
#include <unordered_map>

namespace gui {

class LineLayoutCache {
public:
    const font::LineLayout& getLineLayout(std::string_view str8);
    int maxWidth() const;

private:
    std::unordered_map<XXH64_hash_t, font::LineLayout> cache;

    // TODO: Use a data structure (priority queue) for efficient updating.
    int max_width = 0;
};

}
