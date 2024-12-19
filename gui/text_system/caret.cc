#include "caret.h"

#include "third_party/uni_algo/include/uni_algo/prop.h"
#include <numeric>
#include <optional>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace gui {

namespace {
font::LineLayout::const_iterator IteratorAtColumn(const font::LineLayout& layout, size_t col);
}

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
    auto it = IteratorAtColumn(layout, col);
    if (it != layout.begin()) it--;

    if (layout.begin() != layout.end()) {
        return col - (*it).index;
    } else {
        return 0;
    }
}

size_t Caret::moveToNextGlyph(const font::LineLayout& layout, size_t col) {
    auto it = IteratorAtColumn(layout, col);
    if (it != layout.end()) it++;

    if (it != layout.end()) {
        return (*it).index - col;
    } else {
        return layout.length - col;
    }
}

void Caret::prevWordStart(const base::PieceTree& tree) {
    base::ReverseTreeWalker walker{&tree, index};

    // Move back at least once.
    walker.next_codepoint();

    std::optional<int32_t> prev_cp;
    size_t prev_offset = index;

    while (!walker.exhausted()) {
        size_t offset = walker.offset();
        int32_t cp = walker.next_codepoint();
        if (prev_cp) {
            auto prev_kind = codepointToCharKind(prev_cp.value());
            auto kind = codepointToCharKind(cp);
            if ((prev_kind != kind && prev_kind != CharKind::kWhitespace) || cp == '\n') {
                break;
            }
        }
        prev_cp = cp;
        prev_offset = offset;
    }
    index = prev_offset;
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
            if ((prev_kind != kind && prev_kind != CharKind::kWhitespace) || cp == '\n') {
                break;
            }
        }
        prev_cp = cp;
        prev_offset = walker.offset();
    }
    index = prev_offset;
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

namespace {
font::LineLayout::const_iterator IteratorAtColumn(const font::LineLayout& layout, size_t col) {
    for (auto it = layout.begin(); it != layout.end(); ++it) {
        if ((*it).index >= col) {
            return it;
        }
    }
    return layout.end();
}
}  // namespace

}  // namespace gui
