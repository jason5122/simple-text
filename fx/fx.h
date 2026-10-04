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
    FX_FONT_GLOW = 1u << 8,
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

struct fx_gamma_ramp {
    std::array<uint8_t, 256> values{};
};

class fx_font {
public:
    virtual ~fx_font() = default;

    virtual uint32_t attrs() const = 0;
    virtual fx_font_metrics metrics() const = 0;
    // Unrounded top-to-baseline distance used when the first line is aligned to device pixels.
    virtual float raster_ascent() const = 0;
    virtual std::unique_ptr<fx_layout> shape(std::string_view utf8) = 0;
    std::unique_ptr<fx_layout> shape(std::u32string_view utf32);
    // Reports the scratch-buffer size and the glyph's alphabetic baseline origin within that
    // buffer, both in device pixels with y growing downward.
    virtual void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) = 0;
    // `position` is the device-pixel alphabetic baseline in a buffer allocated from extents().
    //
    // `platform_value` identifies which rasterization parameters to use, and is opaque to fx: the
    // platform layer chooses whatever varies the resulting pixels there, and the glyph cache keeps
    // a separate table per value. Linux passes cairo_subpixel_order_t, supplied by the display.
    // Sublime's Windows build passes a monitor index and selects that monitor's
    // IDWriteRenderingParams with it; we always pass zero, so every display shares the primary
    // monitor's parameters. Core Text ignores it on both.
    virtual void rasterize(uint32_t glyph,
                           vec2 position,
                           float scale,
                           fx_pixel_buffer* buffer,
                           color foreground,
                           uint32_t platform_value) = 0;
    virtual bool is_color_glyph(uint32_t glyph) = 0;
    // Null means the platform does not need gamma correction.
    virtual const fx_gamma_ramp* gamma_ramp() const = 0;

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

    struct glyph_data {
        std::array<glyph_phase, phase_count> phases{};
        bool colored = false;

        const glyph_phase& phase_at(size_t phase) const {
            return phases[phase_count == 1 ? 0 : phase];
        }
    };

    // Core Text's font smoothing changes based on dark/light backgrounds. Renderers must not ask
    // for `alternate` unless this is set.
    static constexpr bool alternate_glyphs = BUILDFLAG(IS_MAC);

    // The largest glyph the cache rasterizes, measured on the scratch buffer with any glow
    // padding included. Past either limit every phase stays empty and the glyph draws as blank,
    // which is also what happens when a phase's bearings do not fit glyph_phase. The side limit
    // is Skia's (SkGlyph::kMaxGlyphWidth): no renderer places a tile that wide. The area limit
    // is the memory bound, 64 MB of scratch, past anything a text UI draws -- 4096 pixels square
    // is a 1000pt face at 2x.
    static constexpr int kMaxGlyphSide = 8192;
    static constexpr int kMaxGlyphPixels = 4096 * 4096;
    // The largest glow radius in device pixels, whatever the face's ascent asks for. It bounds
    // the padding, which a huge ascent would otherwise push past the limits above for every
    // glyph of the font, and the blur, whose cost grows with the radius.
    static constexpr float kMaxGlowRadius = 192.0f;

    // The cache rasterizes through `font` on every miss, so it must outlive the cache.
    fx_glyph_cache(fx_font& font, float scale);
    const glyph_data& lookup_glyph_data(uint32_t glyph,
                                        uint32_t platform_value = 0,
                                        bool alternate = false);

private:
    // Sublime splits the cache one level above the hash: a small integer picks a table, and the
    // glyph alone is the key within it. Exactly one of the two trailing lookup arguments becomes
    // that selector on any given platform, and the other is only a rasterizer input. Mac selects
    // on `alternate`, because Core Text smoothing depends on the background and nothing about a
    // macOS display changes the pixels. Linux and Windows have no live polarity -- their
    // equivalent of bg_affects_rasterise() is constant false -- and select on `platform_value`
    // instead.
    static size_t table_index(uint32_t platform_value, bool alternate);

    fx_font& font_;
    const fx_gamma_ramp* gamma_ramp_ = nullptr;
    float scale_ = 1.0f;
    // Grown on demand, as Sublime does. There is no compile-time bound to size against: the
    // selector is whatever the platform layer supplies, and in Sublime's Windows build that is a
    // monitor index. Reallocating moves the maps, which is safe because a container move is
    // required to be constant time and so cannot relocate nodes -- references handed out by
    // lookup_glyph_data() stay valid.
    std::vector<std::unordered_map<uint32_t, glyph_data>> tables_;
    std::vector<std::unique_ptr<uint32_t[]>> pixel_allocations_;
};

#if BUILDFLAG(IS_WIN)
// Maps a monitor to the `platform_value` that selects its rasterization parameters: the value to
// pass to fx_font::rasterize and fx_glyph_cache::lookup_glyph_data for anything drawn on it.
// `monitor` is an HMONITOR, taken as void* so this header stays free of windows.h.
//
// DirectWrite's gamma, enhanced contrast, ClearType level and pixel geometry are all per-monitor
// settings, so a glyph rasterized for one display is not valid on another. Sublime enumerates the
// display set from the paint arm of its WndProc (0x1401bfc8a), gives each monitor the index it
// enumerates at, and creates that monitor's IDWriteRenderingParams the first time it sees it
// (0x1401cb62e). This mirrors that, lazy creation included.
//
// Returns zero -- the first enumerated monitor's slot -- when `monitor` is null or is not among
// the enumerated displays, and when DirectWrite is unavailable. Call from the thread that paints:
// the mapping is unsynchronized, as Sublime's is.
uint32_t fx_monitor_platform_value(void* monitor);
#endif

// `size` is in logical pixels and must be positive and finite. Null when the platform cannot
// resolve the family, or when the font it resolves reports vertical metrics that are not the
// finite, positive distances fx_font_metrics promises.
std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs);

// Loads face 0 of a TrueType or OpenType file without registering it with the platform, so the
// font is invisible to fx_create_font and to other processes. Characters the file lacks still
// fall back to system fonts, exactly as they do for a named family. Style bits in `attrs` select
// nothing here: a single-face file has no bold or italic variant to pick, and whether a backend
// synthesizes one is platform-dependent.
//
// Null for a file that is not a font the platform can load, and for one whose metrics fail the
// same check as fx_create_font: a malformed file can put its ascent below the baseline or divide
// every metric by a zero unitsPerEm. A glyph the file makes too large to rasterize loads fine
// and draws as blank; see fx_glyph_cache::kMaxGlyphSide and kMaxGlyphPixels.
std::unique_ptr<fx_font> fx_create_font_from_file(std::string_view path,
                                                  float size,
                                                  uint32_t attrs);
