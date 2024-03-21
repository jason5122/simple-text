#pragma once

#include "font/rasterized_glyph.h"
#include <string>

class FreeTypeRasterizer {
public:
    float line_height;
    float descent;

    FreeTypeRasterizer();
    bool setup(std::string main_font_name, int font_size);
    RasterizedGlyph rasterizeUTF8(const char* utf8_str);
    ~FreeTypeRasterizer();

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    impl* pimpl;
    // TODO: Learn and use std::unique_ptr instead.
    // https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
    // std::unique_ptr<impl> pimpl;
};
