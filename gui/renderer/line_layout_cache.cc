#include "base/hash/hash.h"
#include "gui/renderer/line_layout_cache.h"
#include "gui/renderer/renderer.h"

namespace gui {

const editor::LineLayout& LineLayoutCache::get(size_t font_id, std::string_view str8) {
    // The same string laid out in two fonts must not collide.
    uint64_t key = base::hash_combine(font_id, base::hash_string(str8));
    if (auto it = cache.find(key); it != cache.end()) {
        return it->second;
    } else {
        auto& font_cache = Renderer::instance().font_cache();
        auto layout =
            editor::layout_line(font_id, font_cache.font(font_id), FontCache::kScaleFactor, str8);
        auto inserted = cache.emplace(key, std::move(layout));
        return inserted.first->second;
    }
}

void LineLayoutCache::clear() { cache.clear(); }

}  // namespace gui
