#pragma once

#include "font/types.h"
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace font {

class FontRasterizer {
public:
    static FontRasterizer& instance();
    FontRasterizer(const FontRasterizer&) = delete;
    FontRasterizer& operator=(const FontRasterizer&) = delete;

    size_t addFont(std::string_view font_name8, int font_size, FontStyle style = FontStyle::kNone);
    size_t addSystemFont(int font_size, FontStyle style = FontStyle::kNone);
    const Metrics& metrics(size_t font_id) const;

    RasterizedGlyph rasterize(size_t font_id, uint32_t glyph_id) const;
    LineLayout layoutLine(size_t font_id, std::string_view str8);

    // Although public, this is only intended to be called internally.
    // TODO: This is a hack for DirectWrite. Figure out a way to make this private.
    struct NativeFontType;
    size_t cacheFont(NativeFontType font, int font_size);

private:
    FontRasterizer();
    ~FontRasterizer();

    std::unordered_map<std::string, size_t> font_postscript_name_to_id;
    std::vector<NativeFontType> font_id_to_native;
    std::vector<Metrics> font_id_to_metrics;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace font
