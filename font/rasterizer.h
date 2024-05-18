#pragma once

#include "font/rasterized_glyph.h"
#include "util/not_copyable_or_movable.h"
#include <memory>
#include <string>

class FontRasterizer {
public:
    int id;
    float line_height;
    float descent;

    NOT_COPYABLE(FontRasterizer)
    NOT_MOVABLE(FontRasterizer)
    FontRasterizer();
    ~FontRasterizer();

    bool setup(int id, std::string main_font_name, int font_size);
    // TODO: Unify rasterize() methods.
    RasterizedGlyph rasterizeUTF8(std::string_view utf8_str);
    RasterizedGlyph rasterizeTemp(std::string_view utf8_str, uint_least32_t codepoint);

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;
};
