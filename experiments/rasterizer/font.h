#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace font {

// At or below this point size Sublime Text uses a distinct small-size text path: the shaper snaps
// monospace advances to whole points (TextShaper::shape) and the renderer positions glyphs with
// 6-phase horizontal sub-pixel precision (draw_text). Above it, advances stay fractional and
// glyphs snap to whole device pixels.
inline constexpr double kSmallSizeThresholdPt = 16.0;

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

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    // TODO: Use a better method than this.
    friend class FontDatabase;
    friend class TextShaper;
    friend class GlyphRasterizer;
};

enum class Weight { Normal, Bold };
enum class Slant { Normal, Italic };

struct FontRequest {
    std::string family;
    Weight weight = Weight::Normal;
    Slant slant = Slant::Normal;
};

class FontDatabase {
public:
    FontDatabase();
    ~FontDatabase();
    std::optional<FontFaceId> match(const FontRequest& request);
    std::optional<FontHandle> create_font(FontFaceId face, double size_px);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

using GlyphId = uint32_t;

// In device-independent points.
struct GlyphPlacement {
    GlyphId glyph_id = 0;
    double x_advance = 0;
    double y_advance = 0;
    double x_offset = 0;  // pen-relative, top-left
    double y_offset = 0;  // pen-relative, top-left (down is positive)
    size_t cluster = 0;   // byte offset in the UTF-8 source
};

struct ShapedRun {
    FontHandle font;
    std::vector<GlyphPlacement> glyphs;
};

struct ShapedLine {
    std::vector<ShapedRun> runs;
    double width;
};

class TextShaper {
public:
    ShapedLine shape(const FontHandle& font, std::string_view utf8) const;
};

// In device pixels. Dimensions are unsigned: they size `pixels` and feed CoreGraphics, both of
// which take size_t. Bearings are signed because a glyph's ink can sit left of the pen origin or
// above the baseline. The buffer is RGBA8 (4 bytes/pixel), implied by how it's rasterized/blitted.
struct GlyphBitmap {
    size_t width;
    size_t height;
    int bearing_x;
    int bearing_y;
    std::vector<uint8_t> pixels;

    constexpr bool empty() const { return width == 0 || height == 0; }
};

class GlyphRasterizer {
public:
    // subpixel_x is a fractional horizontal offset in device pixels baked into the glyph's
    // antialiasing, letting the caller place the bitmap on an integer pixel while rendering the
    // glyph at a sub-pixel horizontal position.
    GlyphBitmap rasterize(const FontHandle& font,
                          GlyphId glyph,
                          double scale,
                          double subpixel_x = 0.0) const;
};

}  // namespace font
