#include "types.h"
#include <numeric>

namespace font {

std::pair<size_t, int> LineLayout::closestForX(int x) const {
    for (const auto& run : runs) {
        for (const auto& glyph : run.glyphs) {
            int glyph_x = glyph.position.x;
            int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
            if (glyph_center >= x) {
                return {glyph.index, glyph.position.x};
            }
        }
    }
    return {length, width};
}

std::pair<size_t, int> LineLayout::closestForIndex(size_t index) const {
    for (const auto& run : runs) {
        for (const auto& glyph : run.glyphs) {
            if (glyph.index >= index) {
                return {glyph.index, glyph.position.x};
            }
        }
    }
    return {length, width};
}

}
