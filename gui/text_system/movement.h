#pragma once

#include "base/buffer/piece_tree.h"
#include "font/types.h"

#include <cstddef>
#include <string_view>

namespace gui {
namespace movement {

size_t columnAtX(const font::LineLayout& layout, int x);
int xAtColumn(const font::LineLayout& layout, size_t col);

// These return *deltas*.
// TODO: Make documentation more clear. Consider using type aliases.
size_t moveToPrevGlyph(const font::LineLayout& layout, size_t col);
size_t moveToNextGlyph(const font::LineLayout& layout, size_t col);

// These return *offsets*.
// TODO: Make documentation more clear. Consider using type aliases.
size_t prevWordStart(const base::PieceTree& tree, size_t offset);
size_t nextWordEnd(const base::PieceTree& tree, size_t offset);
// TODO: Make a `Pair` type.
std::pair<size_t, size_t> surroundingWord(const base::PieceTree& tree, size_t offset);
bool isInsideWord(const base::PieceTree& tree, size_t offset);

}  // namespace movement
}  // namespace gui
