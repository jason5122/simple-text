#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

class fx_font;

namespace editor {

// A shaped glyph in device pixels, relative to the start of the line. Coordinates are y-down: `x`
// is the pen position along the alphabetic baseline and `y` is the glyph's baseline shift, which
// is 0 for ordinary glyphs and negative for a glyph raised above the baseline.
struct ShapedGlyph {
    // fx glyph ID. The fallback face index is packed into the upper 16 bits.
    uint32_t glyph_id;
    int x;
    int y;
    int advance;
    // UTF-8 byte index of the glyph's cluster in the original text.
    size_t index;
};

struct LineLayout {
    // Opaque handle to the font the glyph IDs belong to; assigned by the caller of layout_line().
    size_t font_id;
    // Total advance in device pixels.
    int width;
    // Length of the original UTF-8 text.
    size_t length;
    std::vector<ShapedGlyph> glyphs;
};

// Shapes `str8` with `font` and converts the result from logical units to device pixels at
// `scale`. `str8` must not contain newlines.
LineLayout layout_line(size_t font_id, fx_font& font, float scale, std::string_view str8);

}  // namespace editor
