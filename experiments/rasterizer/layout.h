#pragma once

#include "experiments/rasterizer/font.h"
#include <map>
#include <vector>

// CPU text layout: shaping + rasterization into the glyph instances and unique bitmaps the GL
// renderer consumes. No GL here -- the atlas upload happens later, when a context exists.

// Identifies a unique rasterized glyph bitmap: the font it was shaped with, its packed glyph (the
// fallback face index and the platform glyph id), the horizontal sub-pixel phase baked into its
// antialiasing (always 0 when sub-pixel positioning is off), and the device-pixel ratio it was
// rasterized at. Two placements sharing a key sample the same atlas cell.
//
// font is an id rather than a pointer on purpose: this outlives the shaping call, and an address
// freed by one font can be handed straight back to the next, which would silently collide keys
// across fonts. An id is never reused, so a stale key just fails to match.
//
// scale is here because the bitmap depends on it and phase alone does not pin it down --
// subpixel_x is phase * scale / 6. One layout_text() call is single-scale, so nothing can collide
// today; this keeps that an invariant of the key rather than of the caller. Sublime quantises the
// equivalent to an integer percentage (scale * 100) because it is a hash key there; an exact
// double costs the same in an ordered map and cannot alias two nearby scales.
struct GlyphKey {
    font::FontId font;
    font::GlyphId glyph;
    int phase;
    double scale;

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
