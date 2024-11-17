#include "caret.h"

#include "third_party/uni_algo/include/uni_algo/prop.h"
#include <numeric>
#include <optional>

namespace gui {

size_t Caret::columnAtX(const font::LineLayout& layout, int x) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;
        int glyph_x = glyph.position.x;
        int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
        if (glyph_center >= x) {
            return glyph.index;
        }
    }
    return layout.length;
}

int Caret::xAtColumn(const font::LineLayout& layout, size_t col) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        const auto& glyph = *it;
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
    // TODO: Fix this.
    // auto it = std::make_reverse_iterator(iteratorAtColumn(layout, col));
    // auto prev_it = layout.rend();  // Invalid/"null" iterator.
    // for (; it != layout.rend(); ++it) {
    //     if (prev_it != layout.rend()) {
    //         std::string_view left_str = line_str.substr((*it).index, (*it).length);
    //         std::string_view right_str = line_str.substr((*prev_it).index, (*prev_it).length);

    //         // TODO: Properly implement this.
    //         bool left_kind = std::isalpha(left_str[0]);
    //         bool right_kind = std::isalpha(right_str[0]);
    //         if (left_kind != right_kind && right_str != " ") {
    //             return col - (*prev_it).index;
    //         }
    //     }
    //     prev_it = it;
    // }
    return col;
}

void Caret::nextWordEnd(const base::PieceTree& tree) {
    base::TreeWalker walker{&tree, index};

    std::optional<int32_t> prev_cp;
    size_t prev_offset = index;

    while (!walker.exhausted()) {
        int32_t cp = walker.next_codepoint();
        if (prev_cp) {
            auto prev_kind = codepointToCharKind(prev_cp.value());
            auto kind = codepointToCharKind(cp);
            bool prev_is_whitespace = una::codepoint::is_whitespace(prev_cp.value());
            if ((prev_kind != kind && !prev_is_whitespace) || cp == '\n') {
                index = prev_offset;
                return;
            }
        }
        prev_cp = cp;
        prev_offset = walker.offset();
    }
}

constexpr CharKind Caret::codepointToCharKind(int32_t codepoint) {
    if (una::codepoint::is_whitespace(codepoint)) {
        return CharKind::kWhitespace;
    } else if (una::codepoint::is_alphabetic(codepoint) || codepoint == '_') {
        return CharKind::kWord;
    } else {
        return CharKind::KPunctuation;
    }
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

}  // namespace gui
