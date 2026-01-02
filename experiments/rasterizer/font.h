#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

struct GlyphPlacement {
    GlyphId glyph = 0;
    double x_advance = 0;
    double y_advance = 0;
    double x_offset = 0;
    double y_offset = 0;
    size_t cluster = 0;  // Byte offset in UTF8.
};

struct ShapedRun {
    FontHandle font;
    std::vector<GlyphPlacement> glyphs;
};

struct ShapedLine {
    std::vector<ShapedRun> runs;
    double width;
    double ascent;
    double descent;
};

class TextShaper {
public:
    ShapedLine shape(const FontHandle& font, std::string_view utf8) const;
};

struct GlyphBitmap {
    size_t width;
    size_t height;
    size_t bytes_per_pixel;
    int bearing_x;
    int bearing_y;
    std::vector<uint8_t> pixels;
};

class GlyphRasterizer {
public:
    GlyphBitmap rasterize(
        const FontHandle& font, GlyphId glyph, double origin_x, double origin_y, int scale) const;
};

}  // namespace font
