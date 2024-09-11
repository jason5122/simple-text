#include "caret.h"
#include <numeric>

namespace gui {

size_t Caret::columnAtX(const font::LineLayout& layout, int x, bool exclude_end) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;
        int glyph_x = glyph.position.x;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return glyph.index;
        }

        int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
        if (glyph_center >= x) {
            return glyph.index;
        }
    }
    return layout.length;
}

int Caret::xAtColumn(const font::LineLayout& layout, size_t col, bool exclude_end) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return glyph.position.x;
        }

        if (glyph.index >= col) {
            return glyph.position.x;
        }
    }
    return layout.width;
}

size_t Caret::moveToPrevGlyph(const font::LineLayout& layout, size_t col) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= col) {
            // TODO: Replace this with saturating sub for iterators.
            if (it != layout.begin()) --it;

            return col - (*it).index;
        }
    }

    if (layout.begin() != layout.end()) {
        auto it = std::prev(layout.end());
        return col - (*it).index;
    } else {
        return 0;
    }
}

size_t Caret::moveToNextGlyph(const font::LineLayout& layout, size_t col) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= col) {
            ++it;

            if (it == layout.end()) {
                return layout.length - col;
            } else {
                return (*it).index - col;
            }
        }
    }
    return layout.length - col;
}

}
