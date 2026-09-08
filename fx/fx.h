#pragma once

#include "base/color.h"
#include "base/geometry.h"
#include "build/build_config.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

enum : uint32_t {
    FX_FONT_BOLD = 1u << 0,
    FX_FONT_ITALIC = 1u << 1,
    FX_FONT_NO_ANTIALIAS = 1u << 2,
    FX_FONT_GRAY_ANTIALIAS = 1u << 3,
    FX_FONT_SUBPIXEL_ANTIALIAS = 1u << 4,
    FX_FONT_NO_ROUND = 1u << 7,
    FX_FONT_NO_LIGA = 1u << 11,
    FX_FONT_NO_CLIG = 1u << 12,
    FX_FONT_NO_CALT = 1u << 13,
    FX_FONT_DLIG = 1u << 14,
    FX_FONT_SS01 = 1u << 15,
    FX_FONT_SS02 = 1u << 16,
    FX_FONT_SS03 = 1u << 17,
    FX_FONT_SS04 = 1u << 18,
    FX_FONT_SS05 = 1u << 19,
    FX_FONT_SS06 = 1u << 20,
    FX_FONT_SS07 = 1u << 21,
    FX_FONT_SS08 = 1u << 22,
    FX_FONT_SS09 = 1u << 23,
    FX_FONT_SS10 = 1u << 24,
};

struct fx_font_metrics {
    // Distances are positive logical units. `ascent` extends upward from the alphabetic baseline,
    // `descent` extends downward, and `leading` is the remaining interline space. Coordinates
    // exposed by fx are otherwise top-left-origin with y growing downward.
    float ascent = 0.0f;
    float descent = 0.0f;
    float leading = 0.0f;
    float line_height = 0.0f;
};

struct fx_font_widths {
    float em_width = 0.0f;
    bool monospace = false;
};

struct fx_glyph {
    uint32_t id = 0;
    // Position relative to the layout's alphabetic baseline in logical y-down coordinates. Thus a
    // normal glyph has y_offset == 0, while an upward displacement has a negative y_offset.
    float x_offset = 0.0f;
    float y_offset = 0.0f;
    uint32_t cluster = 0;
};
static_assert(sizeof(fx_glyph) == 16);

struct fx_layout {
    float advance = 0.0f;
    float line_height = 0.0f;
    std::vector<fx_glyph> glyphs;
};

// Non-owning premultiplied-BGRA raster target. Sublime calls the corresponding type
// px_pixel_buffer; keep the fx prefix here because this view is part of the font interface rather
// than the platform render-context interface.
struct fx_pixel_buffer {
    uint32_t* pixels = nullptr;
    int width = 0;
    int height = 0;
    int row_pixels = 0;
};
static_assert(offsetof(fx_pixel_buffer, pixels) == 0x0);
static_assert(offsetof(fx_pixel_buffer, width) == 0x8);
static_assert(offsetof(fx_pixel_buffer, height) == 0xc);
static_assert(offsetof(fx_pixel_buffer, row_pixels) == 0x10);
static_assert(sizeof(fx_pixel_buffer) == 0x18);

struct fx_gamma_ramp {
    std::array<uint8_t, 256> values{};
    std::array<uint8_t, 256> inverse_values{};
    bool complement_inverse = false;
};

class fx_font {
public:
    virtual ~fx_font() = default;

    virtual uint32_t attrs() const = 0;
    virtual fx_font_metrics metrics() const = 0;
    // Unrounded top-to-baseline distance used when the first line is aligned to device pixels.
    virtual float raster_ascent() const = 0;
    virtual std::unique_ptr<fx_layout> shape(std::string_view utf8) = 0;
    virtual std::unique_ptr<fx_layout> shape(std::u32string_view utf32) = 0;
    // Reports the scratch-buffer size and the glyph's alphabetic baseline origin within that
    // buffer, both in device pixels with y growing downward.
    virtual void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) = 0;
    // `position` is the device-pixel alphabetic baseline in a buffer allocated from extents(). On
    // Linux, `subpixel_order` is cairo_subpixel_order_t's numeric value, supplied by the display.
    // The other native rasterizers ignore it.
    virtual void rasterize(uint32_t glyph,
                           vec2 position,
                           float scale,
                           fx_pixel_buffer* buffer,
                           color foreground,
                           uint32_t subpixel_order) = 0;
    virtual bool is_color_glyph(uint32_t glyph) = 0;
    virtual bool bg_affects_rasterize() const = 0;
    virtual const fx_gamma_ramp* gamma_ramp() const = 0;

    // core_text_font has a UTF-16 overload outside its vtable. Keep that same distinction here.
    std::unique_ptr<fx_layout> shape(std::u16string_view utf16);

    // Lazily shapes M and i. The binary caches these at fx_font+8 and +12.
    fx_font_widths widths();

private:
    bool widths_valid_ = false;
    fx_font_widths widths_;
};

// A cache is constructed for one font and one device scale.
class fx_glyph_cache {
public:
#if BUILDFLAG(IS_LINUX)
    // Pango does not use subpixel positioning.
    static constexpr size_t phase_count = 1;
#else
    static constexpr size_t phase_count = 6;
#endif

    struct glyph_phase {
        uint32_t* pixels = nullptr;
        uint16_t width = 0;
        uint16_t height = 0;
        // Device-pixel offset from the glyph's alphabetic baseline to the cropped bitmap's
        // top-left corner, in y-down coordinates.
        int16_t bearing_x = 0;
        int16_t bearing_y = 0;
    };
    static_assert(offsetof(glyph_phase, pixels) == 0x0);
    static_assert(offsetof(glyph_phase, width) == 0x8);
    static_assert(offsetof(glyph_phase, height) == 0xa);
    static_assert(offsetof(glyph_phase, bearing_x) == 0xc);
    static_assert(offsetof(glyph_phase, bearing_y) == 0xe);
    static_assert(sizeof(glyph_phase) == 16);

    struct glyph_data {
        std::array<glyph_phase, phase_count> phases{};
        bool colored = false;

        const glyph_phase& phase_at(size_t phase) const {
            return phases[phase_count == 1 ? 0 : phase];
        }
    };
#if BUILDFLAG(IS_LINUX)
    static_assert(offsetof(glyph_data, colored) == 0x10);
    static_assert(sizeof(glyph_data) == 0x18);
#else
    static_assert(offsetof(glyph_data, colored) == 0x60);
    static_assert(sizeof(glyph_data) == 0x68);
#endif

    fx_glyph_cache(fx_font* font, float scale);
    const glyph_data& lookup_glyph_data(uint32_t glyph,
                                        uint32_t subpixel_order = 0,
                                        bool alternate = false);

    fx_font* font() const { return font_; }
    float scale() const { return scale_; }

private:
    static uint64_t cache_key(uint32_t glyph, uint32_t subpixel_order);

    fx_font* font_ = nullptr;
    const fx_gamma_ramp* gamma_ramp_ = nullptr;
    float scale_ = 1.0f;
    std::unordered_map<uint64_t, glyph_data> normal_;
    std::unordered_map<uint64_t, glyph_data> alternate_;
    std::vector<std::unique_ptr<uint32_t[]>> pixel_allocations_;
};

// Applies the shared bitmap glow operation used before a glyph is handed to either renderer.
void fx_apply_font_glow(fx_pixel_buffer* buffer, float radius, bool preserve_source);

std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs);
