#include "build/buildflag.h"
#include "glyph_cache.h"

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace renderer {
GlyphCache::GlyphCache(font::FontRasterizer& font_rasterizer) : font_rasterizer(font_rasterizer) {}

void GlyphCache::setup() {
    atlas.setup();
}

AtlasGlyph& GlyphCache::getGlyph(std::string_view key) {
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

AtlasGlyph GlyphCache::createGlyph(std::string_view utf8_str) {
    font::RasterizedGlyph glyph = font_rasterizer.rasterizeUTF8(utf8_str);

    Vec4 uv = atlas.insertTexture(glyph.width, glyph.height, glyph.colored, &glyph.buffer[0]);

    AtlasGlyph atlas_glyph{
        .glyph = Vec4{static_cast<float>(glyph.left), static_cast<float>(glyph.top),
                      static_cast<float>(glyph.width), static_cast<float>(glyph.height)},
        .uv = uv,
        .advance = glyph.advance,
        .colored = glyph.colored,
    };
    return atlas_glyph;
}

void GlyphCache::bindTexture() {
    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);
}

int GlyphCache::lineHeight() {
    return font_rasterizer.line_height;
}
}
