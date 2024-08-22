#pragma once

#include "font/types.h"
#include <memory>
#include <string>

namespace font {

class FontRasterizer {
public:
    static FontRasterizer& instance();

    size_t addFont(const std::string& font_name_utf8, int font_size);

    struct Metrics {
        int font_size;
        int line_height;
        int descent;
        int ascent;
    };
    const Metrics& getMetrics(size_t font_id) const;

    RasterizedGlyph rasterizeUTF8(size_t layout_font_id,
                                  size_t font_id,
                                  uint32_t glyph_id,
                                  float subpixel_variant_x) const;
    LineLayout layoutLine(size_t font_id, std::string_view str8) const;

    // TODO: Make these private. Currently, we make these public as a hack for DirectWrite.
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    FontRasterizer();
    ~FontRasterizer();
};

}
