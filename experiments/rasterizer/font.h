#pragma once

#include "build/build_config.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#if BUILDFLAG(IS_MAC)
using CTFontRef = const struct __CTFont*;
#endif

namespace font {

// Sublime's small-size text path has two cutoffs that sit half a point apart. Both were read off
// Sublime's output at fractional sizes:
//   - The shaper snaps monospace advances to whole points (TextShaper::shape) at 16.0pt and below;
//     16.5pt already keeps its fractional advance.
//   - The renderer positions glyphs with 6-phase horizontal sub-pixel precision (draw_text) below
//     17pt, so 16.5pt still gets it but 17.0pt does not. Above the cutoff, glyphs snap to whole
//     device pixels.
inline constexpr double kMonospaceSnapMaxPt = 16.0;
inline constexpr double kSubpixelMaxPt = 16.5;

using FontFaceId = uint32_t;

class FontHandle {
public:
    FontHandle() = default;
    ~FontHandle();
    FontHandle(FontHandle&&);
    FontHandle& operator=(FontHandle&&);
    FontHandle(const FontHandle&) = delete;
    FontHandle& operator=(const FontHandle&) = delete;

    bool valid() const;
    double ascent() const;
    double descent() const;
    double leading() const;

    // TODO: Remove this. If we need a unique key, consider adding PostScript name.
    const void* native_handle() const;

#if BUILDFLAG(IS_MAC)
    explicit FontHandle(CTFontRef ct_font);
    CTFontRef ct_font() const;
#endif

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

using GlyphId = uint32_t;

// In device-independent points.
struct GlyphPlacement {
    GlyphId glyph_id = 0;
    double x_advance = 0;
    double x_offset = 0;
    double y_offset = 0;
    size_t cluster = 0;
};

struct ShapedRun {
    FontHandle font;
    std::vector<GlyphPlacement> glyphs;
};

struct ShapedLine {
    std::vector<ShapedRun> runs;
    double width;
};

// In device pixels.
struct GlyphBitmap {
    size_t width;
    size_t height;
    int bearing_x;
    int bearing_y;
    // True for color glyphs (e.g. emoji): `pixels` holds real RGBA color and must be drawn as-is.
    // False for normal glyphs, whose coverage is in the alpha channel (RGB is 0), so they can be
    // tinted any color.
    bool is_color = false;
    std::vector<uint8_t> pixels;

    constexpr bool empty() const { return width == 0 || height == 0; }
};

enum class Weight { Normal, Bold };
enum class Slant { Normal, Italic };

std::optional<FontHandle> create_font(std::string family,
                                      double size_px,
                                      Weight weight = Weight::Normal,
                                      Slant slant = Slant::Normal);
ShapedLine shape(const FontHandle& font, std::string_view utf8);
GlyphBitmap rasterize(const FontHandle& font,
                      GlyphId glyph,
                      double scale,
                      double subpixel_x = 0.0);

}  // namespace font
