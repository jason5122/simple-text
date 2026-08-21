#pragma once

#include "experiments/rasterizer/font.h"
#include <map>
#include <string>
#include <vector>

// CPU text layout: shaping + rasterization into the glyph instances and unique bitmaps the GL
// renderer consumes. No GL here -- the atlas upload happens later, when a context exists.

// Identifies a unique rasterized glyph bitmap: the font it came from (FontHandle::cache_key(), not
// native_handle() -- a run's fallback FontHandle is often a short-lived temporary, and its
// underlying CTFontRef pointer can be reused by an unrelated font once freed, which would collide
// keys across fonts), its glyph id, and the horizontal sub-pixel phase baked into its antialiasing
// (always 0 when sub-pixel positioning is off). Two placements sharing a key sample the same atlas
// cell.
struct GlyphKey {
    std::string font;
    font::GlyphId glyph;
    int phase;

    auto operator<=>(const GlyphKey&) const = default;
};

// One glyph to draw, positioned on the device-pixel grid with a top-left origin. The bearing is
// already folded into dst_x/dst_y, so this is the top-left corner of the glyph bitmap.
struct GlyphInstance {
    GlyphKey key;
    int dst_x;
    int dst_y;
};

// Everything the GL renderer needs to draw one page of text: the glyphs to draw and, keyed the
// same way, the unique bitmaps to pack into the atlas.
struct GlyphAtlasSource {
    std::vector<GlyphInstance> instances;
    std::map<GlyphKey, font::GlyphBitmap> bitmaps;
};

// Shapes and rasterizes every line with `font` at device-pixel ratio `scale`.
GlyphAtlasSource layout_text(const font::FontHandle& font,
                             const std::vector<std::string>& lines,
                             double scale);
