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
    impl* pimpl;
    // TODO: Learn and use std::unique_ptr instead.
    // https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
    // std::unique_ptr<impl> pimpl;
};
