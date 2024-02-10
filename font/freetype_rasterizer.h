#pragma once

#include "font/types/rasterized_glyph.h"
#include <freetype/freetype.h>
#include <hb.h>
#include <vector>

class FreeTypeRasterizer {
public:
    float line_height;
    float descent;

    FreeTypeRasterizer() = default;
    bool setup(const char* font_path, int font_size);
    RasterizedGlyph rasterizeUTF8(const char* utf8_str);
    ~FreeTypeRasterizer();

private:
    std::vector<std::pair<FT_Face, hb_font_t*>> font_fallback_list;

    std::pair<hb_codepoint_t, size_t> getGlyphIndex(const char* utf8_str);
};
