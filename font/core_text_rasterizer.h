#pragma once

#include "font/rasterized_glyph.h"

class CoreTextRasterizer {
public:
    float line_height;
    float descent;

    CoreTextRasterizer();
    void setup(std::string main_font_name, int font_size);
    RasterizedGlyph rasterizeUTF8(const char* utf8_str);
    ~CoreTextRasterizer();

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    impl* pimpl;
    // TODO: Learn and use std::unique_ptr instead.
    // https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
    // std::unique_ptr<impl> pimpl;
};
