#pragma once

#include "font/types/rasterized_glyph.h"
#include <freetype/freetype.h>
#include <hb.h>

class FreeTypeRasterizer {
public:
    float line_height;
    float descent;

    FreeTypeRasterizer() = default;
    bool setup(const char* font_path);
    RasterizedGlyph rasterizeUTF8(const char* utf8_str);
    ~FreeTypeRasterizer();

private:
    FT_Face face;
    hb_font_t* harfbuzz_font;

    hb_codepoint_t getGlyphIndex(const char* utf8_str);
};
