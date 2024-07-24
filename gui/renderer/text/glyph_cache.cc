#include "glyph_cache.h"

namespace gui {

GlyphCache::GlyphCache(const std::string& font_name_utf8, int font_size)
    : font_rasterizer{font_name_utf8, font_size} {
    atlas_pages.emplace_back();
}

GlyphCache::Glyph& GlyphCache::getGlyph(size_t font_id, uint32_t glyph_id) {
    if (!cache[font_id].contains(glyph_id)) {
        font::FontRasterizer::RasterizedGlyph rglyph =
            font_rasterizer.rasterizeUTF8(font_id, glyph_id);

        cache[font_id].emplace(glyph_id, loadGlyph(std::move(rglyph)));
    }
    return cache[font_id][glyph_id];
}

GlyphCache::Glyph GlyphCache::loadGlyph(const font::FontRasterizer::RasterizedGlyph& rglyph) {
    Atlas& atlas = atlas_pages[current_page];

    // TODO: Handle the case when a texture is too large for the atlas.
    //       Return an enum classifying the error instead of using a boolean.
    Vec4 uv;
    bool success =
        atlas.insertTexture(rglyph.width, rglyph.height, rglyph.colored, rglyph.buffer, uv);

    // The current page is full, so create a new page and try again.
    if (!success) {
        atlas_pages.emplace_back();
        current_page++;

        return loadGlyph(rglyph);
    }

    Glyph glyph{
        .tex_id = atlas.tex(),
        .glyph = Vec4{static_cast<float>(rglyph.left), static_cast<float>(rglyph.top),
                      static_cast<float>(rglyph.width), static_cast<float>(rglyph.height)},
        .uv = uv,
        .advance = rglyph.advance,
        .colored = rglyph.colored,
        .page = current_page,
    };
    return glyph;
}

int GlyphCache::lineHeight() const {
    return font_rasterizer.getLineHeight();
}

const font::FontRasterizer& GlyphCache::rasterizer() const {
    return font_rasterizer;
}

}
