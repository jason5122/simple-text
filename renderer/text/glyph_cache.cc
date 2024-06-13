#include "glyph_cache.h"

namespace renderer {

GlyphCache::GlyphCache(opengl::FunctionsGL* gl, const std::string& font_name_utf8, int font_size)
    : gl{gl}, font_rasterizer{font_name_utf8, font_size} {}

void GlyphCache::setup() {
    Atlas atlas{gl};
    atlas.setup();
    atlas_pages.push_back(std::move(atlas));
}

GlyphCache::Glyph& GlyphCache::getGlyph(std::string_view str8) {
    // If the input is ASCII (a very common occurrence), we can skip the overhead of a hash map.
    if (str8.length() == 1 && 0x20 <= str8[0] && str8[0] <= 0x7e) {
        size_t i = str8[0] - 0x20;
        if (ascii_cache.at(i) == std::nullopt) {
            font::RasterizedGlyph rglyph = font_rasterizer.rasterizeUTF8(str8);
            ascii_cache.at(i) = loadGlyph(rglyph);
        }
        return ascii_cache.at(i).value();
    }

    auto it = cache.find(str8);
    if (it == cache.end()) {
        font::RasterizedGlyph rglyph = font_rasterizer.rasterizeUTF8(str8);
        it = cache.emplace(str8, loadGlyph(rglyph)).first;
    }
    return it->second;
}

GlyphCache::Glyph GlyphCache::loadGlyph(const font::RasterizedGlyph& rglyph) {
    Atlas& atlas = atlas_pages[current_page];

    // TODO: Handle the case when a texture is too large for the atlas.
    //       Return an enum classifying the error instead of using a boolean.
    Vec4 uv;
    bool success =
        atlas.insertTexture(rglyph.width, rglyph.height, rglyph.colored, &rglyph.buffer[0], uv);

    // The current page is full, so create a new page and try again.
    if (!success) {
        Atlas new_atlas{gl};
        new_atlas.setup();
        atlas_pages.push_back(std::move(new_atlas));
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

int GlyphCache::lineHeight() {
    return font_rasterizer.line_height;
}

}
