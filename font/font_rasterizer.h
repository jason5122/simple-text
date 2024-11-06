#pragma once

#include "font/types.h"
#include <memory>
#include <string>

namespace font {

class FontRasterizer {
public:
    static FontRasterizer& instance();

    size_t addFont(std::string_view font_name_utf8,
                   int font_size,
                   FontStyle style = FontStyle::kNone);
    const Metrics& getMetrics(size_t font_id) const;

    RasterizedGlyph rasterize(size_t font_id, uint32_t glyph_id) const;
    LineLayout layoutLine(size_t font_id, std::string_view str8) const;

    // TODO: Make these private. Currently, we make these public as a hack for DirectWrite.
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    FontRasterizer();
    ~FontRasterizer();
};

}  // namespace font
