#include "rasterizer.h"

class FontRasterizer::impl {
public:
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    return true;
}

RasterizedGlyph FontRasterizer::rasterizeUTF8(const char* utf8_str) {
    return {};
}

std::vector<RasterizedGlyph> FontRasterizer::layoutLine(const char* utf8_str) {
    return {};
}

FontRasterizer::~FontRasterizer() {}
