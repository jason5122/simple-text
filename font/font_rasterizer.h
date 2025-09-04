#pragma once

#include "font/types.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace font {

using FontId = size_t;

class FontRasterizer {
public:
    static FontRasterizer& instance();
    FontRasterizer(const FontRasterizer&) = delete;
    FontRasterizer& operator=(const FontRasterizer&) = delete;
    FontRasterizer(FontRasterizer&&) = delete;
    FontRasterizer& operator=(FontRasterizer&&) = delete;

    FontId add_font(std::string_view font_name8,
                    int font_size,
                    FontStyle font_style = FontStyle::kNone);
    FontId add_system_font(int font_size, FontStyle font_style = FontStyle::kNone);
    // TODO: Clean this up. Consider keeping the same font ID or change method name from "resize"
    // to "create copy".
    FontId resize_font(FontId font_id, int font_size);
    const Metrics& metrics(FontId font_id) const;
    std::string_view postscript_name(FontId font_id) const;

    RasterizedGlyph rasterize(FontId font_id, uint32_t glyph_id) const;
    LineLayout layout_line(FontId font_id, std::string_view str8);

private:
    friend class Impl;

    FontRasterizer();
    ~FontRasterizer() noexcept;

    struct NativeFontType;
    std::unordered_map<size_t, FontId> font_hash_to_id;
    std::vector<NativeFontType> font_id_to_native;
    std::vector<Metrics> font_id_to_metrics;
    std::vector<std::string> font_id_to_postscript_name;

    // TODO: This is a hack for DirectWrite. Find a way to make this private.
public:
    size_t hash_font(std::string_view font_name8, int font_size) const;
    size_t hash_font(std::wstring_view font_name16, int font_size) const;
    FontId cache_font(NativeFontType native_font, int font_size);

    class Impl;
    std::unique_ptr<Impl> pimpl;
};

static_assert(!std::is_copy_constructible_v<FontRasterizer>);
static_assert(!std::is_copy_assignable_v<FontRasterizer>);
static_assert(!std::is_move_constructible_v<FontRasterizer>);
static_assert(!std::is_move_assignable_v<FontRasterizer>);

}  // namespace font
