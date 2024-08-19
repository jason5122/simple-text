#include "caret.h"
#include <numeric>

namespace gui {

void Caret::moveToX(const font::LineLayout& layout, size_t line, int x, bool exclude_end) {
    this->line = line;
    auto set = [&](size_t index, int x) {
        this->index = index;
        this->x = x;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;
        int glyph_x = glyph.position.x;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return set(glyph.index, glyph_x);
        }

        int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
        if (glyph_center >= x) {
            return set(glyph.index, glyph_x);
        }
    }
    set(layout.length, layout.width);
}

void Caret::moveToIndex(const font::LineLayout& layout,
                        size_t line,
                        size_t index,
                        bool exclude_end) {
    this->line = line;
    auto set = [&](size_t index, int x) {
        this->index = index;
        this->x = x;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return set(glyph.index, glyph.position.x);
        }

        if (glyph.index >= index) {
            return set(glyph.index, glyph.position.x);
        }
    }
    set(layout.length, layout.width);
}

size_t Caret::moveToPrevGlyph(const font::LineLayout& layout, size_t line, size_t index) {
    this->line = line;
    auto set = [&](size_t index, int x) {
        size_t delta = this->index - index;
        this->index = index;
        this->x = x;
        return delta;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= index) {
            // TODO: Replace this with saturating sub for iterators.
            if (it != layout.begin()) --it;

            return set((*it).index, (*it).position.x);
        }
    }

    if (layout.begin() != layout.end()) {
        auto it = std::prev(layout.end());
        const auto& glyph = *it;
        return set(glyph.index, glyph.position.x);
    } else {
        return 0;
    }
}

size_t Caret::moveToNextGlyph(const font::LineLayout& layout,
                              size_t line,
                              size_t index,
                              bool exclude_end) {
    this->line = line;
    auto set = [&](size_t index, int x) {
        size_t delta = this->index - index;
        this->index = index;
        this->x = x;
        return delta;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return set(glyph.index, glyph.position.x);
        }

        if (glyph.index >= index) {
            ++it;

            if (it == layout.end()) {
                return set(layout.length, layout.width);
            } else {
                return set((*it).index, (*it).position.x);
            }
        }
    }
    return set(layout.length, layout.width);
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
