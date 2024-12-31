#include "glyph_cache.h"

namespace gui {

GlyphCache::GlyphCache() {
    atlas_pages.emplace_back();
}

const GlyphCache::Glyph& GlyphCache::getGlyph(size_t font_id, uint32_t glyph_id) {
    if (cache.size() <= font_id) {
        cache.resize(font_id + 1);
    }

    if (!cache[font_id].contains(glyph_id)) {
        const auto& font_rasterizer = font::FontRasterizer::instance();
        auto rglyph = font_rasterizer.rasterize(font_id, glyph_id);
        cache[font_id].emplace(glyph_id, insertIntoAtlas(std::move(rglyph)));
    }
    return cache[font_id][glyph_id];
}

// TODO: Refactor recursion.
GlyphCache::Glyph GlyphCache::insertIntoAtlas(const font::RasterizedGlyph& rglyph) {
    Atlas& atlas = atlas_pages[current_page];

    // TODO: Handle the case when a texture is too large for the atlas.
    //       Return an enum classifying the error instead of using a boolean.
    Vec4 uv;
    bool success =
        atlas.insertTexture(rglyph.width, rglyph.height, Atlas::Format::kBGRA, rglyph.buffer, uv);

    // The current page is full, so create a new page and try again.
    if (!success) {
        atlas_pages.emplace_back();
        ++current_page;
        return insertIntoAtlas(rglyph);
    }

    return {
        .left = rglyph.left,
        .top = rglyph.top,
        .width = rglyph.width,
        .height = rglyph.height,
        .uv = uv,
        .colored = rglyph.colored,
        .page = current_page,
    };
}

const std::vector<Atlas>& GlyphCache::pages() const {
    return atlas_pages;
}

}  // namespace gui
