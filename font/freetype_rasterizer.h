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
    FT_Face ft_main_face;
    std::vector<hb_font_t*> font_fallback_list;

    hb_codepoint_t getGlyphIndex(const char* utf8_str);
};
