#include "movement.h"

#include "third_party/uni_algo/include/uni_algo/prop.h"

#include <numeric>
#include <optional>
#include <vector>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace gui {

namespace {

enum class CharKind {
    kWhitespace,
    kWord,
    KPunctuation,
};

constexpr CharKind CodepointToCharKind(int32_t codepoint);

}  // namespace

size_t Movement::columnAtX(const font::LineLayout& layout, int x) {
    for (size_t i = 0; i < layout.glyphs.size(); ++i) {
        const auto& glyph = layout.glyphs[i];
        int glyph_x = glyph.position.x;
        int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
        if (glyph_center >= x) {
            return glyph.index;
        }
    }
    return layout.length;
}

int Movement::xAtColumn(const font::LineLayout& layout, size_t col) {
    for (size_t i = 0; i < layout.glyphs.size(); ++i) {
        const auto& glyph = layout.glyphs[i];
        if (glyph.index >= col) {
            return glyph.position.x;
        }
    }
    return layout.width;
}

// TODO: Refactor this! Don't use iterators at all.
namespace {
auto IteratorAtColumn(const font::LineLayout& layout, size_t col) {
    for (auto it = layout.glyphs.begin(); it != layout.glyphs.end(); ++it) {
        if ((*it).index >= col) {
            return it;
        }
    }
    return layout.glyphs.end();
}
}  // namespace

size_t Movement::moveToPrevGlyph(const font::LineLayout& layout, size_t col) {
    auto it = IteratorAtColumn(layout, col);
    if (it != layout.glyphs.begin()) it--;

    if (layout.glyphs.begin() != layout.glyphs.end()) {
        return col - (*it).index;
    } else {
        return 0;
    }
}

size_t Movement::moveToNextGlyph(const font::LineLayout& layout, size_t col) {
    auto it = IteratorAtColumn(layout, col);
    if (it != layout.glyphs.end()) it++;

    if (it != layout.glyphs.end()) {
        return (*it).index - col;
    } else {
        return layout.length - col;
    }
}

size_t Movement::prevWordStart(const base::PieceTree& tree, size_t offset) {
    base::ReverseTreeWalker reverse_walker{&tree, offset};

    std::optional<int32_t> prev_cp;
    size_t prev_offset = offset;

    while (!reverse_walker.exhausted()) {
        int32_t cp = reverse_walker.next_codepoint();
        size_t offset = reverse_walker.offset();

        // TODO: Properly handle errors.
        if (cp == -1) {
            fmt::println("Movement::prevWordStart() error: invalid codepoint.");
            std::abort();
        }

        if (prev_cp) {
            auto prev_kind = CodepointToCharKind(prev_cp.value());
            auto kind = CodepointToCharKind(cp);
            if ((prev_kind != kind && prev_kind != CharKind::kWhitespace) || cp == '\n') {
                break;
            }
        }
        prev_cp = cp;
        prev_offset = offset;
    }
    return prev_offset;
}

size_t Movement::nextWordEnd(const base::PieceTree& tree, size_t offset) {
    base::TreeWalker walker{&tree, offset};

    std::optional<int32_t> prev_cp;
    size_t prev_offset = offset;

    while (!walker.exhausted()) {
        int32_t cp = walker.next_codepoint();
        size_t offset = walker.offset();

        // TODO: Properly handle errors.
        if (cp == -1) {
            fmt::println("Movement::nextWordEnd() error: invalid codepoint.");
            std::abort();
        }

        if (prev_cp) {
            auto prev_kind = CodepointToCharKind(prev_cp.value());
            auto kind = CodepointToCharKind(cp);
            if ((prev_kind != kind && prev_kind != CharKind::kWhitespace) || cp == '\n') {
                break;
            }
        }
        prev_offset = offset;
        prev_cp = cp;
    }
    return prev_offset;
}

namespace {

constexpr CharKind CodepointToCharKind(int32_t codepoint) {
    if (una::codepoint::is_whitespace(codepoint)) {
        return CharKind::kWhitespace;
    } else if (una::codepoint::is_alphabetic(codepoint) || codepoint == '_') {
        return CharKind::kWord;
    } else {
        return CharKind::KPunctuation;
    }
}

}  // namespace

}  // namespace gui
