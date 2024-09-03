#pragma once

#include "font/types.h"
#include <cstddef>

namespace gui {

struct Caret {
    size_t index;

    static size_t columnAtX(const font::LineLayout& layout, int x, bool exclude_end = false);
    static int xAtColumn(const font::LineLayout& layout, size_t col, bool exclude_end = false);

    size_t moveToPrevGlyph(const font::LineLayout& layout, size_t col);
    size_t moveToNextGlyph(const font::LineLayout& layout, size_t col);

    friend constexpr bool operator==(const Caret& c1, const Caret& c2) {
        return c1.index == c2.index;
    }
    friend constexpr auto operator<=>(const Caret& c1, const Caret& c2) {
        return c1.index <=> c2.index;
    }
};

}
