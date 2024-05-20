#pragma once

#include "util/not_copyable_or_movable.h"
#include <memory>
#include <string>
#include <vector>

namespace font {
struct RasterizedGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    float advance;
    std::vector<uint8_t> buffer;
    // TODO: Either remove these debug fields, or evaluate if we should keep them.
    unsigned short index;
};

class FontRasterizer {
public:
    int id;
    int line_height;
    int descent;

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
}
