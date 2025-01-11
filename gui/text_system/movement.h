#pragma once

#include "base/buffer/piece_tree.h"
#include "font/types.h"

#include <cstddef>
#include <string_view>

namespace gui {

class Movement {
public:
    static size_t columnAtX(const font::LineLayout& layout, int x);
    static int xAtColumn(const font::LineLayout& layout, size_t col);

    // These return *deltas*.
    // TODO: Make documentation more clear. Consider using type aliases.
    static size_t moveToPrevGlyph(const font::LineLayout& layout, size_t col);
    static size_t moveToNextGlyph(const font::LineLayout& layout, size_t col);

    // These return *offsets*.
    // TODO: Make documentation more clear. Consider using type aliases.
    static size_t prevWordStart(const base::PieceTree& tree, size_t offset);
    static size_t nextWordEnd(const base::PieceTree& tree, size_t offset);
    // TODO: Make a `Pair` type.
    static std::pair<size_t, size_t> surroundingWord(const base::PieceTree& tree, size_t offset);
    static bool isInsideWord(const base::PieceTree& tree, size_t offset);
};

}  // namespace gui
