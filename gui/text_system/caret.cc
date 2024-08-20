#include "caret.h"
#include <numeric>

namespace gui {

void Caret::moveToX(const font::LineLayout& layout, int x, bool exclude_end) {
    auto set = [&](size_t col, int x) {
        this->index = col;
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

void Caret::moveToIndex(const font::LineLayout& layout, size_t col, bool exclude_end) {
    auto set = [&](size_t col, int x) {
        this->index = col;
        this->x = x;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return set(glyph.index, glyph.position.x);
        }

        if (glyph.index >= col) {
            return set(glyph.index, glyph.position.x);
        }
    }
    set(layout.length, layout.width);
}

size_t Caret::moveToPrevGlyph(const font::LineLayout& layout, size_t col) {
    auto set = [&](size_t index, int x) {
        size_t delta = 0;
        // size_t delta = this->col - index;
        // this->col = index;
        // this->x = x;
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

size_t Caret::moveToNextGlyph(const font::LineLayout& layout, size_t col, bool exclude_end) {
    auto set = [&](size_t index, int x) {
        size_t delta = 0;
        // size_t delta = this->col - index;
        // this->col = index;
        // this->x = x;
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

}
