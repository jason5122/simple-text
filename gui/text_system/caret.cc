#include "caret.h"
#include <numeric>

namespace gui {

size_t Caret::columnAtX(const font::LineLayout& layout, float x, bool exclude_end) const {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(layout.end())) {
            return glyph.index;
        }

        float glyph_center = std::midpoint(glyph.position.x, glyph.position.x + glyph.advance.x);
        if (glyph_center >= x) {
            return glyph.index;
        }
    }
    return layout.length;
}

float Caret::xAtColumn(const font::LineLayout& layout, size_t col, bool exclude_end) const {
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

size_t Caret::moveToPrevGlyph(const font::LineLayout& layout, size_t col) {
    auto set = [&](size_t index) {
        size_t delta = col - index;
        this->index -= delta;
        return delta;
    };

    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= col) {
            // TODO: Replace this with saturating sub for iterators.
            if (it != layout.begin()) --it;

            return set((*it).index);
        }
    }

    if (layout.begin() != layout.end()) {
        auto it = std::prev(layout.end());
        const auto& glyph = *it;
        return set(glyph.index);
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
