#pragma once

#include "font/types.h"
#include <memory>
#include <string>

namespace font {

class FontRasterizer {
public:
    FontRasterizer(const std::string& font_name_utf8, int font_size);
    ~FontRasterizer();

    RasterizedGlyph rasterizeUTF8(size_t font_id, uint32_t glyph_id) const;
    LineLayout layoutLine(std::string_view str8) const;
    int getLineHeight() const;

    // TODO: Make these private. Currently, we make these public as a hack for DirectWrite.
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    int line_height;
    int descent;
};

}
