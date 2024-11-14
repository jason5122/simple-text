#pragma once

#include "font/types.h"
#include <cstddef>
#include <string_view>

namespace gui {

class Caret {
public:
    size_t index = 0;

    static size_t columnAtX(const font::LineLayout& layout, int x);
    static int xAtColumn(const font::LineLayout& layout, size_t col);

    static size_t moveToPrevGlyph(const font::LineLayout& layout, size_t col);
    static size_t moveToNextGlyph(const font::LineLayout& layout, size_t col);

    static size_t prevWordStart(const font::LineLayout& layout,
                                size_t col,
                                std::string_view line_str);
    static size_t nextWordEnd(const font::LineLayout& layout,
                              size_t col,
                              std::string_view line_str);

    friend constexpr bool operator==(const Caret& c1, const Caret& c2) {
        return c1.index == c2.index;
    }
    friend constexpr bool operator!=(const Caret& c1, const Caret& c2) {
        return !(c1.index == c2.index);
    }
    friend constexpr bool operator<(const Caret& c1, const Caret& c2) {
        return c1.index < c2.index;
    }
    friend constexpr bool operator>(const Caret& c1, const Caret& c2) {
        return c2.index > c1.index;
    }
    friend constexpr bool operator<=(const Caret& c1, const Caret& c2) {
        return !(c1.index > c2.index);
    }
    friend constexpr bool operator>=(const Caret& c1, const Caret& c2) {
        return !(c1.index < c2.index);
    }

private:
    static font::LineLayout::const_iterator iteratorAtColumn(const font::LineLayout& layout,
                                                             size_t col);
};

}  // namespace gui
