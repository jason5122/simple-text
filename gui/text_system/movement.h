#pragma once

#include "base/buffer/piece_tree.h"
#include "font/types.h"

#include <cstddef>
#include <string_view>

namespace gui {
namespace movement {

size_t column_at_x(const font::LineLayout& layout, int x);
int x_at_column(const font::LineLayout& layout, size_t col);

// These return *deltas*.
// TODO: Make documentation more clear. Consider using type aliases.
size_t move_to_prev_glyph(const font::LineLayout& layout, size_t col);
size_t move_to_next_glyph(const font::LineLayout& layout, size_t col);

// These return *offsets*.
// TODO: Make documentation more clear. Consider using type aliases.
size_t prev_word_start(const base::PieceTree& tree, size_t offset);
size_t next_word_end(const base::PieceTree& tree, size_t offset);
// TODO: Make a `Pair` type.
std::pair<size_t, size_t> surrounding_word(const base::PieceTree& tree, size_t offset);
bool is_inside_word(const base::PieceTree& tree, size_t offset);

}  // namespace movement
}  // namespace gui
