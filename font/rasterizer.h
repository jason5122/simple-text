#pragma once

#include "font/types/rasterized_glyph.h"

class Rasterizer {
public:
    float line_height;
    float descent;

    Rasterizer() = default;
    Rasterizer(std::string main_font_name, std::string emoji_font_name, int font_size);
    RasterizedGlyph rasterizeUTF8(const char* utf8_str);
    bool isFontMonospace();
    uint16_t getGlyphIndex(const char* utf8_str);
    uint16_t getEmojiGlyphIndex(const char* utf8_str);

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    impl* pimpl;
    // TODO: Learn and use std::unique_ptr instead.
    // https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
    // std::unique_ptr<impl> pimpl;
};
