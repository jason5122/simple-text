#include "line_layout_cache.h"

#include "base/hash/hash_combine.h"
#include "gui/renderer/renderer.h"
#include "third_party/hash_maps/rapidhash.h"

namespace gui {

const font::LineLayout& LineLayoutCache::get(size_t font_id, std::string_view str8) {
    uint64_t hash = rapidhash(str8.data(), str8.length());
    if (auto it = cache.find(hash); it != cache.end()) {
        return it->second;
    } else {
        auto& font_rasterizer = font::FontRasterizer::instance();
        auto layout = font_rasterizer.layoutLine(font_id, str8);
        auto inserted = cache.emplace(hash, std::move(layout));
        return inserted.first->second;
    }
}

}  // namespace gui
