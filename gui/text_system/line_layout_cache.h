#pragma once

#include "font/types.h"
#include <string_view>
#include <unordered_map>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-builtins"
#include "third_party/robin_hood/robin_hood.h"
#pragma clang diagnostic pop

namespace gui {

class LineLayoutCache {
public:
    LineLayoutCache(size_t font_id);

    const font::LineLayout& get(std::string_view str8, int font_size);
    int maxWidth() const;

private:
    size_t font_id;
    robin_hood::unordered_map<size_t, font::LineLayout> cache;
};

}  // namespace gui
