#include "glyph_cache.h"

namespace gui {

GlyphCache::GlyphCache() {
    atlas_pages.emplace_back();
}

const GlyphCache::Glyph& GlyphCache::getGlyph(size_t font_id,
                                              uint32_t glyph_id,
                                              const font::FontRasterizer& font_rasterizer) {
    if (cache.size() <= font_id) {
        cache.resize(font_id + 1);
    }

    if (!cache[font_id].contains(glyph_id)) {
        auto rglyph = font_rasterizer.rasterize(font_id, glyph_id);
        cache[font_id].emplace(glyph_id, loadGlyph(std::move(rglyph)));
    }
    return cache[font_id][glyph_id];
}

GlyphCache::Glyph GlyphCache::loadGlyph(const font::RasterizedGlyph& rglyph) {
    Atlas& atlas = atlas_pages[current_page];

    // TODO: Handle the case when a texture is too large for the atlas.
    //       Return an enum classifying the error instead of using a boolean.
    Vec4 uv;
    bool success =
        atlas.insertTexture(rglyph.width, rglyph.height, rglyph.colored, rglyph.buffer, uv);

    // The current page is full, so create a new page and try again.
    if (!success) {
        atlas_pages.emplace_back();
        ++current_page;

        return loadGlyph(rglyph);
    }

    Glyph glyph{
        .tex_id = atlas.tex(),
        .glyph = Vec4{static_cast<float>(rglyph.left), static_cast<float>(rglyph.top),
                      static_cast<float>(rglyph.width), static_cast<float>(rglyph.height)},
        .uv = uv,
        .colored = rglyph.colored,
        .page = current_page,
    };
    return glyph;
}

void GlyphCache::setMainFontId(size_t font_id) {
    // TODO: Refactor this ugly hack.
    while (cache.size() <= font_id) {
        cache.emplace_back();
    }
    main_font_id = font_id;
}

void GlyphCache::setUIFontId(size_t font_id) {
    // TODO: Refactor this ugly hack.
    while (cache.size() <= font_id) {
        cache.emplace_back();
    }
    ui_font_id = font_id;
}

size_t GlyphCache::mainFontId() const {
    return main_font_id;
}

size_t GlyphCache::uiFontId() const {
    return ui_font_id;
}

const std::vector<Atlas>& GlyphCache::atlasPages() const {
    return atlas_pages;
}

}  // namespace gui
