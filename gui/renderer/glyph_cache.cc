#include "glyph_cache.h"

namespace gui {

GlyphCache::GlyphCache() {
    atlas_pages.emplace_back();
}

GlyphCache::Glyph& GlyphCache::getGlyph(size_t layout_font_id,
                                        size_t font_id,
                                        uint32_t glyph_id,
                                        const font::FontRasterizer& font_rasterizer) {
    // TODO: Refactor this ugly hack.
    while (cache[layout_font_id].size() <= font_id) {
        cache[layout_font_id].emplace_back();
    }

    if (!cache[layout_font_id][font_id].contains(glyph_id)) {
        auto rglyph = font_rasterizer.rasterizeUTF8(font_id, glyph_id);
        cache[layout_font_id][font_id].emplace(glyph_id, loadGlyph(std::move(rglyph)));
    }
    return cache[layout_font_id][font_id][glyph_id];
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

}
