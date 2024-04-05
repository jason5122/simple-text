#pragma once

#include "font/rasterized_glyph.h"
#include <string>

class FontRasterizer {
public:
    int id;
    float line_height;
    float descent;

    FontRasterizer();
    bool setup(int id, std::string main_font_name, int font_size);
    // TODO: Unify rasterize() methods.
    RasterizedGlyph rasterizeUTF8(const char* utf8_str);
    RasterizedGlyph rasterizeTemp(std::string& utf8_str, uint_least32_t codepoint);
    std::vector<RasterizedGlyph> layoutLine(const char* utf8_str);
    ~FontRasterizer();

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;
};
