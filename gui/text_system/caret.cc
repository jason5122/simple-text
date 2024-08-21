#include "caret.h"
#include <numeric>

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

namespace gui {

size_t Caret::columnAtX(const font::LineLayout& layout, int x, bool exclude_end) const {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;
        int glyph_x = glyph.position.x;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return glyph.index;
        }

        int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
        if (glyph_center >= x) {
            return glyph.index;
        }
    }
    return layout.length;
}

int Caret::xAtColumn(const font::LineLayout& layout, size_t col, bool exclude_end) const {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return glyph.position.x;
        }

        if (glyph.index >= col) {
            return glyph.position.x;
        }
    }
    return layout.width;
}

void Caret::moveToX(const font::LineLayout& layout, int x, bool exclude_end) {
    auto set = [&](size_t col, int x) {
        this->index = col;
        // this->x = x;
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
        // this->x = x;
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
        size_t delta = col - index;
        this->index -= delta;
        return delta;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= col) {
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

size_t Caret::moveToNextGlyph(const font::LineLayout& layout, size_t col) {
    auto set = [&](size_t index) {
        size_t delta = index - col;
        this->index += delta;
        return delta;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= col) {
            ++it;

            if (it == layout.end()) {
                return set(layout.length);
            } else {
                return set((*it).index);
            }
        }
    }
    return set(layout.length);
}

}
