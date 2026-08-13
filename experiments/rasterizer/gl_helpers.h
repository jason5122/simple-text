#pragma once

#include "experiments/rasterizer/font.h"
#include <map>
#include <tuple>
#include <vector>

// Identifies a unique rasterized glyph bitmap: the font it came from, its glyph id, and the
// horizontal sub-pixel phase baked into its antialiasing (always 0 when sub-pixel positioning is
// off). Two placements sharing a key sample the same atlas cell.
struct GlyphKey {
    const void* font;
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

// Everything the GL renderer needs: the glyphs to draw and, keyed the same way, the unique bitmaps
// to pack into the atlas. Rasterization runs on the CPU while the fonts are alive; the atlas
// upload is deferred until a GL context exists (the layer's first draw).
struct GlyphAtlasSource {
    std::vector<GlyphInstance> instances;
    std::map<GlyphKey, font::GlyphBitmap> bitmaps;
};

// Opens a window and draws each glyph as its own textured quad sampled from a glyph atlas, plus
// the whole atlas in the upper-right corner as a debug view. `scale` is the device-pixel ratio the
// glyphs were positioned at, used as the layer's contents scale.
void show_window_gl(const GlyphAtlasSource& source, double scale);
