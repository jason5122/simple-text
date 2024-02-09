#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

class FreeTypeRasterizer {
public:
    FreeTypeRasterizer() = default;
    bool setup(const char* font_path);
    void rasterizeUTF8(const char* utf8_str);

private:
    FT_Face face;
};
