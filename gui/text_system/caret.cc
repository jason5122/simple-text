#include "caret.h"
#include <numeric>

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

namespace gui {

size_t Caret::columnAtX(const font::LineLayout& layout, int x, bool exclude_end) {
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

int Caret::xAtColumn(const font::LineLayout& layout, size_t col, bool exclude_end) {
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
    auto it = iteratorAtColumn(layout, col);
    if (it != layout.begin()) it--;

    if (layout.begin() != layout.end()) {
        return col - (*it).index;
    } else {
        return 0;
    }
}

size_t Caret::moveToNextGlyph(const font::LineLayout& layout, size_t col) {
    auto it = iteratorAtColumn(layout, col);
    if (it != layout.end()) it++;

    if (it != layout.end()) {
        return (*it).index - col;
    } else {
        return layout.length - col;
    }
}

size_t Caret::prevWordStart(const font::LineLayout& layout,
                            size_t col,
                            std::string_view line_str) {
    auto it = std::make_reverse_iterator(iteratorAtColumn(layout, col));
    auto prev_it = layout.rend();  // Invalid/"null" iterator.
    for (; it != layout.rend(); ++it) {
        if (prev_it != layout.rend()) {
            std::string_view left_str = line_str.substr((*it).index, (*it).length);
            std::string_view right_str = line_str.substr((*prev_it).index, (*prev_it).length);

            // TODO: Properly implement this.
            bool left_kind = std::isalpha(left_str[0]);
            bool right_kind = std::isalpha(right_str[0]);
            if (left_kind != right_kind && right_str != " ") {
                return col - (*prev_it).index;
            }
        }
        prev_it = it;
    }
    return col;
}

size_t Caret::nextWordEnd(const font::LineLayout& layout, size_t col, std::string_view line_str) {
    auto it = iteratorAtColumn(layout, col);
    auto prev_it = layout.end();  // Invalid/"null" iterator.
    for (; it != layout.end(); ++it) {
        if (prev_it != layout.end()) {
            std::string_view left_str = line_str.substr((*prev_it).index, (*prev_it).length);
            std::string_view right_str = line_str.substr((*it).index, (*it).length);

            // TODO: Properly implement this.
            bool left_kind = std::isalpha(left_str[0]);
            bool right_kind = std::isalpha(right_str[0]);
            if (left_kind != right_kind && left_str != " ") {
                return (*it).index - col;
            }
        }
        prev_it = it;
    }
    return layout.length - col;
}

font::LineLayout::const_iterator Caret::iteratorAtColumn(const font::LineLayout& layout,
                                                         size_t col) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        if ((*it).index >= col) {
            return it;
        }
    }
    return layout.end();
}

}
