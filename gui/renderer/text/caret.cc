#include "caret.h"

namespace gui {

void Caret::moveToX(const font::LineLayout& layout, size_t line, int x) {
    auto [glyph_index, glyph_x] = layout.closestForX(x);
    this->line = line;
    this->index = glyph_index;
    this->x = glyph_x;
}

void Caret::moveToIndex(const font::LineLayout& layout, size_t line, size_t index) {
    auto [glyph_index, glyph_x] = layout.closestForIndex(index);
    this->line = line;
    this->index = glyph_index;
    this->x = glyph_x;
}

void Caret::moveToPrevGlyph(const font::LineLayout& layout, size_t line, size_t index) {
    auto [glyph_index, glyph_x] = layout.prevClosestForIndex(index);
    this->line = line;
    this->index = glyph_index;
    this->x = glyph_x;
}

void Caret::moveToNextGlyph(const font::LineLayout& layout, size_t line, size_t index) {
    auto [glyph_index, glyph_x] = layout.nextClosestForIndex(index);
    this->line = line;
    this->index = glyph_index;
    this->x = glyph_x;
}

// TODO: Move this to tests. Also, test all comparison operators.
static_assert(Caret{0, 0} < Caret{0, 1});
static_assert(Caret{0, 1} < Caret{1, 0});
static_assert(Caret{1, 0} < Caret{1, 1});
static_assert(!(Caret{1, 0} < Caret{1, 0}));

static_assert(Caret{0, 1} > Caret{0, 0});
static_assert(Caret{1, 0} > Caret{0, 1});
static_assert(Caret{1, 1} > Caret{1, 0});
static_assert(!(Caret{1, 0} > Caret{1, 0}));

}
