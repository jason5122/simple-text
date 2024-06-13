#pragma once

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

class FontRasterizer {
public:
    int line_height;
    int descent;

    FontRasterizer(const std::string& font_name_utf8, int font_size);
    ~FontRasterizer();

    RasterizedGlyph rasterizeUTF8(std::string_view str8);

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
