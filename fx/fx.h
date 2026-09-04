// Sublime Text's renderer-independent font mechanics.
//
// The interface is reconstructed from the fx_font/core_text_font vtables in ST 4200. Platform
// font implementations shape text and rasterize one glyph; the shared layer owns layout values,
// width classification, gamma metadata, and CPU glyph caches. Renderer-specific caches (such as
// gl_glyph_cache and its texture atlases) intentionally live above this directory.

#pragma once

#include "base/color.h"
#include "base/geometry.h"

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
};

// Four floats, matching core_text_font::metrics()'s register return in the binary.
struct fx_font_metrics {
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
    float x_offset = 0.0f;
    float y_offset = 0.0f;
    uint32_t cluster = 0;
};
static_assert(sizeof(fx_glyph) == 16);

// ST stores one flat array. Fallback-face identity is packed into the upper half of each glyph id,
// so a layout does not need separate font runs.
struct fx_layout {
    float advance = 0.0f;
    float line_height = 0.0f;
    std::vector<fx_glyph> glyphs;
};

// Premultiplied BGRA in host byte order. The shared cache sets `colored` after asking the native
// font whether the glyph has intrinsic color; native rasterizers only paint the supplied buffer.
struct fx_glyph_bitmap {
    size_t width = 0;
    size_t height = 0;
    int bearing_x = 0;
    int bearing_y = 0;
    bool colored = false;
    std::vector<uint8_t> pixels;

    bool empty() const { return width == 0 || height == 0; }
};

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
    virtual void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) = 0;
    // `position` is a device-pixel offset into a buffer allocated from extents().
    virtual void rasterize(
        uint32_t glyph, vec2 position, float scale, fx_glyph_bitmap& bitmap, color foreground) = 0;
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

// One cache is constructed for one font and one device scale. Its two internal maps correspond to
// ST's normal and alternate/background-dependent cache slots. The renderer chooses the slot from
// its text/background policy; retaining the split keeps that renderer-facing contract intact.
class fx_glyph_cache {
public:
    fx_glyph_cache(fx_font* font, float scale);
    const fx_glyph_bitmap& lookup_glyph_data(uint32_t glyph,
                                             unsigned phase,
                                             bool alternate = false);

    fx_font* font() const { return font_; }
    float scale() const { return scale_; }

private:
    static uint64_t key(uint32_t glyph, unsigned phase);

    fx_font* font_ = nullptr;
    float scale_ = 1.0f;
    std::unordered_map<uint32_t, bool> color_glyphs_;
    std::unordered_map<uint64_t, fx_glyph_bitmap> normal_;
    std::unordered_map<uint64_t, fx_glyph_bitmap> alternate_;
};

// Applies the shared bitmap glow operation used before a glyph is handed to either renderer.
void fx_apply_font_glow(fx_glyph_bitmap* bitmap, float radius, bool preserve_source);

// Creates the native implementation (Core Text on macOS, DirectWrite on Windows). Returns null if
// the requested family cannot be resolved. "system" uses the native UI font.
std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs);
