#include "base/hash/hash.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/line_layout_cache.h"

namespace gui {

const font::LineLayout& LineLayoutCache::get(size_t font_id, std::string_view str8) {
    uint64_t str_hash = base::hash_string(str8);
    if (auto it = cache.find(str_hash); it != cache.end()) {
        return it->second;
    } else {
        auto& font_rasterizer = font::FontRasterizer::instance();
        auto layout = font_rasterizer.layout_line(font_id, str8);
        auto inserted = cache.emplace(str_hash, std::move(layout));
        return inserted.first->second;
    }
}

void LineLayoutCache::clear() { cache.clear(); }

}  // namespace gui
