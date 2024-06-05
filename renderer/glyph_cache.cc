#include "glyph_cache.h"

namespace renderer {

GlyphCache::GlyphCache(font::FontRasterizer& font_rasterizer) : font_rasterizer(font_rasterizer) {}

void GlyphCache::setup() {
    atlas.setup();
}

GlyphCache::Glyph& GlyphCache::getGlyph(std::string_view key) {
    // If the input is ASCII (a very common occurrence), we can skip the overhead of a hash map.
    if (key.length() == 1 && 0x20 <= key[0] && key[0] <= 0x7e) {
        size_t i = key[0] - 0x20;
        if (ascii_cache.at(i) == std::nullopt) {
            ascii_cache.at(i) = createGlyph(key);
        }
        return ascii_cache.at(i).value();
    }

    auto it = cache.find(key);
    if (it == cache.end()) {
        it = cache.emplace(key, createGlyph(key)).first;
    }
    return it->second;
}

GlyphCache::Glyph GlyphCache::createGlyph(std::string_view str8) {
    font::RasterizedGlyph rglyph = font_rasterizer.rasterizeUTF8(str8);

    Vec4 uv;
    atlas.insertTexture(rglyph.width, rglyph.height, rglyph.colored, &rglyph.buffer[0], uv);

    Glyph glyph{
        .glyph = Vec4{static_cast<float>(rglyph.left), static_cast<float>(rglyph.top),
                      static_cast<float>(rglyph.width), static_cast<float>(rglyph.height)},
        .uv = uv,
        .advance = rglyph.advance,
        .colored = rglyph.colored,
    };
    return glyph;
}

void GlyphCache::bindTexture() {
    glBindTexture(GL_TEXTURE_2D, atlas.tex());
}

int GlyphCache::lineHeight() {
    return font_rasterizer.line_height;
}

}
