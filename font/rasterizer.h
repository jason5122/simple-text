#pragma once

#include "util/non_copyable.h"
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
    int32_t advance;
    std::vector<uint8_t> buffer;
};

class FontRasterizer : util::NonMovable {
public:
    int line_height;
    int descent;

    FontRasterizer(const std::string& font_name_utf8, int font_size);
    ~FontRasterizer();

    RasterizedGlyph rasterizeUTF8(std::string_view str8);

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
