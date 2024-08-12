#include "gui/renderer/renderer.h"
#include "line_layout_cache.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <format>
#include <iostream>

namespace gui {

const font::LineLayout& LineLayoutCache::getLineLayout(std::string_view str8) {
    if (auto it = cache.find(str8); it != cache.end()) {
        return it->second;
    } else {
        GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();

        auto layout = main_glyph_cache.rasterizer().layoutLine(str8);
        auto inserted = cache.emplace(str8, std::move(layout));

        // TODO: Track max width in a better way.
        max_width = std::max(layout.width, max_width);

        return inserted.first->second;
    }
}

int LineLayoutCache::maxWidth() const {
    return max_width;
}

}
