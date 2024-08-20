#pragma once

#include "font/types.h"
#include <cstddef>

namespace gui {

struct Caret {
    size_t index;

    // Cached values.
    // size_t line;
    // size_t col;  // UTF-8 index in line.
    int x;
    // We use this value to position the caret during vertical movement.
    // This is updated whenever the caret moves horizontally.
    int max_x;

    void moveToX(const font::LineLayout& layout, int x, bool exclude_end = false);
    void moveToIndex(const font::LineLayout& layout, size_t col, bool exclude_end = false);
    size_t moveToPrevGlyph(const font::LineLayout& layout, size_t col);
    size_t moveToNextGlyph(const font::LineLayout& layout, size_t col, bool exclude_end = false);

    friend constexpr bool operator==(const Caret& c1, const Caret& c2) {
        return c1.index == c2.index;
    }
    friend constexpr auto operator<=>(const Caret& c1, const Caret& c2) {
        return c1.index <=> c2.index;
    }
};

}
