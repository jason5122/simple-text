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
    // TODO: Clean this up. Consider keeping the same font ID or change method name from "resize"
    // to "create copy".
    size_t resizeFont(size_t font_id, int font_size);
    const Metrics& metrics(size_t font_id) const;

    RasterizedGlyph rasterize(size_t font_id, uint32_t glyph_id) const;
    LineLayout layoutLine(size_t font_id, std::string_view str8);

private:
    friend class impl;

    FontRasterizer();
    ~FontRasterizer();

    struct NativeFontType;
    std::unordered_map<size_t, size_t> font_hash_to_id;
    std::vector<NativeFontType> font_id_to_native;
    std::vector<Metrics> font_id_to_metrics;
    size_t hashFont(std::string_view font_name8, int font_size) const;
    size_t cacheFont(NativeFontType font, int font_size);

    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace font
