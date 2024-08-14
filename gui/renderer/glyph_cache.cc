#include "glyph_cache.h"

namespace gui {

GlyphCache::GlyphCache(const std::string& main_font_name_utf8,
                       int main_font_size,
                       const std::string& ui_font_name_utf8,
                       int ui_font_size)
    : main_font_rasterizer{main_font_name_utf8, main_font_size},
      ui_font_rasterizer{ui_font_name_utf8, ui_font_size} {
    atlas_pages.emplace_back();
}

GlyphCache::Glyph& GlyphCache::getMainGlyph(size_t font_id, uint32_t glyph_id) {
    if (!main_cache[font_id].contains(glyph_id)) {
        auto rglyph = main_font_rasterizer.rasterizeUTF8(font_id, glyph_id);
        main_cache[font_id].emplace(glyph_id, loadGlyph(std::move(rglyph)));
    }
    return main_cache[font_id][glyph_id];
}

GlyphCache::Glyph& GlyphCache::getUIGlyph(size_t font_id, uint32_t glyph_id) {
    if (!ui_cache[font_id].contains(glyph_id)) {
        auto rglyph = ui_font_rasterizer.rasterizeUTF8(font_id, glyph_id);
        ui_cache[font_id].emplace(glyph_id, loadGlyph(std::move(rglyph)));
    }
    return ui_cache[font_id][glyph_id];
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
        .advance = rglyph.advance,
        .colored = rglyph.colored,
        .page = current_page,
    };
    return glyph;
}

int GlyphCache::mainLineHeight() const {
    return main_font_rasterizer.getLineHeight();
}

int GlyphCache::uiLineHeight() const {
    return ui_font_rasterizer.getLineHeight();
}

const font::FontRasterizer& GlyphCache::mainRasterizer() const {
    return main_font_rasterizer;
}

const font::FontRasterizer& GlyphCache::uiRasterizer() const {
    return ui_font_rasterizer;
}

}
