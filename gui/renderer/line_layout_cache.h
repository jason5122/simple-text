#pragma once

#include "font/types.h"

#include "third_party/hash_maps/robin_hood.h"

#include <string_view>

namespace gui {

class LineLayoutCache {
public:
    const font::LineLayout& get(size_t font_id, std::string_view str8);

private:
    // We use a node-based map since we need to keep references stable.
    robin_hood::unordered_node_map<uint64_t, font::LineLayout> cache;
};

}  // namespace gui
