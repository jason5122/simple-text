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
    double size() const;
    double ascent() const;
    double descent() const;
    double leading() const;
    // True if the font is fixed-pitch, by Sublime's metric: 'i' and 'M' advance within 0.001pt.
    // Computed once and cached.
    bool is_monospace() const;

    const void* native_handle() const;

    // Stable identity for glyph-cache keys: PostScript name + point size. Unlike native_handle(),
    // this stays valid after the FontHandle it came from is destroyed, so it's safe to cache
    // rasterized glyphs (which outlive any one shaped run's temporary fallback-font handles) by it.
    std::string cache_key() const;

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

// In device pixels.
struct GlyphBitmap {
    size_t width;
    size_t height;
    int bearing_x;
    int bearing_y;
    bool colored;
    std::vector<uint8_t> pixels;

    constexpr bool empty() const { return width == 0 || height == 0; }
};

enum class Weight { Normal, Bold };
enum class Slant { Normal, Italic };

// A requested font, i.e. everything create_font() needs. Handy for callers that
// carry a font around and mutate it (e.g. changing size or face at runtime).
struct FontSpec {
    std::string family;
    double size = 14.0;
    Weight weight = Weight::Normal;
    Slant slant = Slant::Normal;
};

std::optional<FontHandle> create_font(std::string family,
                                      double size_px,
                                      Weight weight = Weight::Normal,
                                      Slant slant = Slant::Normal);
std::optional<FontHandle> create_font(const FontSpec& spec);

std::vector<ShapedRun> shape(const FontHandle& font, std::string_view utf8);

GlyphBitmap rasterize(const FontHandle& font,
                      GlyphId glyph,
                      double scale,
                      double subpixel_x = 0.0);

}  // namespace font
