#pragma once

#include "font/types.h"
#include <cstddef>

namespace gui {

struct Caret {
    size_t line;
    size_t col;  // UTF-8 index in line.
    int x;
    // We use this value to position the caret during vertical movement.
    // This is updated whenever the caret moves horizontally.
    int max_x;

    void moveToX(const font::LineLayout& layout, size_t line, int x, bool exclude_end = false);
    void moveToIndex(const font::LineLayout& layout,
                     size_t line,
                     size_t index,
                     bool exclude_end = false);
    size_t moveToPrevGlyph(const font::LineLayout& layout, size_t line, size_t index);
    size_t moveToNextGlyph(const font::LineLayout& layout,
                           size_t line,
                           size_t index,
                           bool exclude_end = false);

    friend constexpr bool operator==(const Caret& c1, const Caret& c2) {
        return c1.line == c2.line && c1.col == c2.col;
    }

    friend constexpr bool operator!=(const Caret& c1, const Caret& c2) {
        return !(operator==(c1, c2));
    }

    friend constexpr bool operator<(const Caret& c1, const Caret& c2) {
        if (c1.line == c2.line) {
            return c1.col < c2.col;
        } else {
            return c1.line < c2.line;
        }
    }

    friend constexpr bool operator>(const Caret& c1, const Caret& c2) {
        return operator<(c2, c1);
    }

    friend constexpr bool operator<=(const Caret& c1, const Caret& c2) {
        return !(operator>(c1, c2));
    }

    friend constexpr bool operator>=(const Caret& c1, const Caret& c2) {
        return !(operator<(c1, c2));
    }
};

}
