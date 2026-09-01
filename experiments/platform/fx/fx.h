// Sublime Text's renderer-independent font mechanics.
//
// The interface is reconstructed from the fx_font/core_text_font vtables in ST 4200. Platform
// font implementations shape text and rasterize one glyph; the shared layer owns layout values,
// width classification, gamma metadata, and CPU glyph caches. Renderer-specific caches (such as
// gl_glyph_cache and its texture atlases) intentionally live above this directory.

#pragma once

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
};

struct fx_vec2 {
    float x = 0.0f;
    float y = 0.0f;
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
    float advance = 0.0f;
    float x_offset = 0.0f;
    // Fixed-pitch layout can snap the caret while the glyph keeps its native raster position.
    float raster_x_delta = 0.0f;
    float y_offset = 0.0f;
    // DirectWrite snaps each fallback face's ascent at raster scale. Keeping it here lets the
    // renderer preserve that device-pixel correction after shaping has established run baselines.
    float face_ascent = 0.0f;
    size_t cluster = 0;
};

// ST stores one flat array. Fallback-face identity is packed into the upper half of each glyph id,
// so a layout does not need separate font runs.
struct fx_layout {
    float advance = 0.0f;
    float line_height = 0.0f;
    float primary_y_offset = 0.0f;
    float primary_face_ascent = 0.0f;
    std::vector<fx_glyph> glyphs;
};

struct fx_layout_batch {
    double x_offset = 0.0;
    fx_layout layout;
};

// Premultiplied BGRA in host byte order, exactly what the proven rasterizer backend produces and
// what the GL texture upload consumes. For a monochrome glyph RGB contains coverage; for a color
// glyph it contains premultiplied color.
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
    virtual bool extents(uint32_t glyph, float scale, fx_vec2* origin, fx_vec2* size) = 0;
    virtual fx_glyph_bitmap rasterise(uint32_t glyph, float scale, float subpixel_x) = 0;
    virtual bool is_color_glyph(uint32_t glyph) = 0;
    virtual bool bg_affects_rasterise() const = 0;
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
    struct glyph_data {
        fx_glyph_bitmap bitmap;
    };

    fx_glyph_cache(fx_font* font, float scale);
    const glyph_data& lookup_glyph_data(uint32_t glyph, unsigned phase, bool alternate = false);

    fx_font* font() const { return font_; }
    float scale() const { return scale_; }

private:
    static uint64_t key(uint32_t glyph, unsigned phase);

    fx_font* font_ = nullptr;
    float scale_ = 1.0f;
    std::unordered_map<uint64_t, glyph_data> normal_;
    std::unordered_map<uint64_t, glyph_data> alternate_;
};

// Compact single-phase cache used by px_font_t for measurements and small UI glyphs in ST.
class fx_mini_glyph_cache {
public:
    fx_mini_glyph_cache(fx_font* font, float scale) : cache_(font, scale) {}
    const fx_glyph_cache::glyph_data& lookup_glyph_data(uint32_t glyph) {
        return cache_.lookup_glyph_data(glyph, 0, false);
    }

private:
    fx_glyph_cache cache_;
};

// Applies the shared bitmap glow operation used before a glyph is handed to either renderer.
void fx_apply_font_glow(fx_glyph_bitmap* bitmap, float radius, bool preserve_source);

// Creates the native implementation (Core Text on macOS, DirectWrite on Windows). Returns null if
// the requested family cannot be resolved. "system" uses the native UI font.
std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs);

// Shapes text using the policy recovered from Sublime's grapheme_shaper rather than passing the
// entire string to the platform shaper. Combining sequences and emoji stay together, while
// ordinary characters are shaped independently; monospace operator runs are the one exception so
// programming ligatures can form. ST also rounds monospace advances through 16 pt, controlled by
// snap_monospace_advances here because fx_font itself deliberately does not store the requested
// point size.
std::vector<fx_layout_batch> fx_shape_graphemes(fx_font* font,
                                                std::string_view utf8,
                                                bool snap_monospace_advances);
