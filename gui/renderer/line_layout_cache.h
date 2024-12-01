#pragma once

#include "font/types.h"
#include <string_view>
#include <unordered_map>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-builtins"
#include "third_party/hash_maps/robin_hood.h"
#pragma clang diagnostic pop

namespace gui {

class LineLayoutCache {
public:
    const font::LineLayout& get(size_t font_id, std::string_view str8);

private:
    robin_hood::unordered_map<uint64_t, font::LineLayout> cache;
};

}  // namespace gui
