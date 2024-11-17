#pragma once

#include "base/buffer/piece_tree.h"
#include "font/types.h"
#include <cstddef>
#include <string_view>

namespace gui {

enum class CharKind {
    kWhitespace,
    kWord,
    KPunctuation,
};

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
    void nextWordEnd(const base::PieceTree& tree);

    static constexpr CharKind codepointToCharKind(int32_t codepoint);

    friend constexpr bool operator==(const Caret& c1, const Caret& c2) {
        return c1.index == c2.index;
    }
    friend constexpr bool operator!=(const Caret& c1, const Caret& c2) {
        return c1.index != c2.index;
    }
    friend constexpr bool operator<(const Caret& c1, const Caret& c2) {
        return c1.index < c2.index;
    }
    friend constexpr bool operator>(const Caret& c1, const Caret& c2) {
        return c1.index > c2.index;
    }
    friend constexpr bool operator<=(const Caret& c1, const Caret& c2) {
        return c1.index <= c2.index;
    }
    friend constexpr bool operator>=(const Caret& c1, const Caret& c2) {
        return c1.index >= c2.index;
    }

private:
    static font::LineLayout::const_iterator iteratorAtColumn(const font::LineLayout& layout,
                                                             size_t col);
};

}  // namespace gui
