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

    void prevWordStart(const base::PieceTree& tree);
    void nextWordEnd(const base::PieceTree& tree);

    static constexpr CharKind codepointToCharKind(int32_t codepoint);

    // https://brevzin.github.io/c++/2019/07/28/comparisons-cpp20/
    constexpr auto operator<=>(const Caret& rhs) const = default;
};

}  // namespace gui
