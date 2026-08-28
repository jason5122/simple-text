#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace font {

// Identifies the font a glyph was shaped with. Process-unique and never reused, so a key left in a
// cache after its font dies fails to match rather than matching whatever was allocated next -- the
// reason this is an id and not a pointer.
using FontId = uint32_t;

// Index into a font's fallback face registry; 0 is the face the font itself resolved to.
using FontFaceId = uint16_t;

// A shaped glyph, packed the way Sublime packs it: the fallback face index in the high 16 bits and
// the platform glyph id in the low 16. A face that shaping fell back to has no requested family to
// name it, so it is identified positionally within the font that discovered it -- which is what
// lets a glyph carry its own face instead of a font object travelling alongside it.
using GlyphId = uint32_t;

constexpr FontFaceId face_index_of(GlyphId glyph) { return static_cast<FontFaceId>(glyph >> 16); }
constexpr uint16_t glyph_index_of(GlyphId glyph) { return static_cast<uint16_t>(glyph & 0xFFFF); }
constexpr GlyphId pack_glyph(FontFaceId face, uint16_t glyph) {
    return (static_cast<GlyphId>(face) << 16) | glyph;
}

// In device-independent points. ST stores each fx_layout entry as float32, so convert Core Text's
// CGFloat values at this boundary rather than after accumulating them as doubles.
struct GlyphPlacement {
    GlyphId glyph_id = 0;  // packed; see pack_glyph
    float x_advance = 0;
    float x_offset = 0;
    float y_offset = 0;
    size_t cluster = 0;  // byte offset into the shaped UTF-8
};

// The result of shaping, mirroring Sublime's fx_layout: one flat glyph array with no per-run
// grouping, because each glyph already names its own face.
struct ShapedText {
    float advance = 0;      // total advance of the whole string
    float line_height = 0;  // ascent + descent + leading, rounded as the backend rounds them
    std::vector<GlyphPlacement> glyphs;
};

// In device pixels.
struct GlyphBitmap {
    size_t width = 0;
    size_t height = 0;
    int bearing_x = 0;
    int bearing_y = 0;
    bool colored = false;
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

class FontHandle {
public:
    FontHandle();
    ~FontHandle();
    FontHandle(FontHandle&&);
    FontHandle& operator=(FontHandle&&);
    FontHandle(const FontHandle&) = delete;
    FontHandle& operator=(const FontHandle&) = delete;

    bool valid() const;
    FontId id() const;
    double size() const;
    double ascent() const;
    double descent() const;
    double leading() const;
    // True if the font is fixed-pitch, by Sublime's metric: 'i' and 'M' advance within 0.001pt.
    // Computed once and cached.
    bool is_monospace() const;

    // Per-font state shared by every handle onto the same font: the platform font, the metrics,
    // and the append-only fallback face registry. Each backend defines its own; public only so the
    // backend's free functions can reach it.
    struct FontData;
    explicit FontHandle(std::shared_ptr<FontData> data);
    FontData& data() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

std::optional<FontHandle> create_font(std::string family,
                                      double size_px,
                                      Weight weight = Weight::Normal,
                                      Slant slant = Slant::Normal);
#if defined(__APPLE__)
std::optional<FontHandle> create_font(
    std::string family, double size_px, Weight weight, Slant slant, uint32_t feature_flags);
#endif
std::optional<FontHandle> create_font(const FontSpec& spec);

ShapedText shape(const FontHandle& font, std::string_view utf8);

GlyphBitmap rasterize(const FontHandle& font,
                      GlyphId glyph,
                      double scale,
                      double subpixel_x = 0.0);

// Describes how the platform rasteriser is currently configured, for --dump-glyph. Exists because
// the settings that shape antialiasing (DirectWrite's gamma, contrast and ClearType level) are
// otherwise invisible from outside the backend.
std::string rasterizer_debug_info();

// Debug: selects DirectWrite's CreateGlyphRunAnalysis instead of its bitmap render target. The two
// are different mask sources -- the render target applies colour-dependent gamma and contrast, the
// alpha texture returns raw coverage. No effect on other platforms.
void set_debug_use_analysis_path(bool enabled);

// Debug: overrides the gamma and enhanced contrast handed to the rasteriser. A negative gamma
// restores the backend's own choice. No effect on other platforms.
void set_debug_rendering_params(float gamma, float contrast);

}  // namespace font
