#include "movement.h"

#include "third_party/uni_algo/include/uni_algo/prop.h"

#include <numeric>
#include <optional>
#include <vector>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace gui {
namespace movement {

namespace {

enum class CharKind {
    kWhitespace,
    kPunctuation,
    kWord,
};

constexpr CharKind to_kind(int32_t codepoint);

}  // namespace

size_t columnAtX(const font::LineLayout& layout, int x) {
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

int xAtColumn(const font::LineLayout& layout, size_t col) {
    for (size_t i = 0; i < layout.glyphs.size(); ++i) {
        const auto& glyph = layout.glyphs[i];
        if (glyph.index >= col) {
            return glyph.position.x;
        }
    }
    return layout.width;
}

namespace {
inline size_t GlyphAtColumn(const std::vector<font::ShapedGlyph>& glyphs, size_t col) {
    for (size_t i = 0; i < glyphs.size(); ++i) {
        if (glyphs[i].index >= col) {
            return i;
        }
    }
    return glyphs.size();
}
}  // namespace

size_t moveToPrevGlyph(const font::LineLayout& layout, size_t col) {
    const auto& glyphs = layout.glyphs;
    if (glyphs.empty()) return 0;

    size_t i = GlyphAtColumn(glyphs, col);
    if (i > 0) --i;
    return col - glyphs[i].index;
}

size_t moveToNextGlyph(const font::LineLayout& layout, size_t col) {
    const auto& glyphs = layout.glyphs;

    size_t i = GlyphAtColumn(glyphs, col);
    if (i < glyphs.size()) ++i;

    if (i < glyphs.size()) {
        return glyphs[i].index - col;
    } else {
        return layout.length - col;
    }
}

size_t prevWordStart(const base::PieceTree& tree, size_t offset) {
    base::ReverseTreeWalker reverse_walker{&tree, offset};

    std::optional<int32_t> prev_cp;
    size_t prev_offset = offset;

    while (!reverse_walker.exhausted()) {
        int32_t cp = reverse_walker.next_codepoint();
        size_t offset = reverse_walker.offset();

        // TODO: Properly handle errors.
        if (cp == -1) {
            fmt::println("prevWordStart() error: invalid codepoint.");
            std::abort();
        }

        if (prev_cp) {
            auto prev_kind = to_kind(prev_cp.value());
            auto kind = to_kind(cp);
            if ((prev_kind != kind && prev_kind != CharKind::kWhitespace) || cp == '\n') {
                break;
            }
        }
        prev_cp = cp;
        prev_offset = offset;
    }
    return prev_offset;
}

size_t nextWordEnd(const base::PieceTree& tree, size_t offset) {
    base::TreeWalker walker{&tree, offset};

    std::optional<int32_t> prev_cp;
    size_t prev_offset = offset;

    while (!walker.exhausted()) {
        int32_t cp = walker.next_codepoint();
        size_t offset = walker.offset();

        // TODO: Properly handle errors.
        if (cp == -1) {
            fmt::println("nextWordEnd() error: invalid codepoint.");
            std::abort();
        }

        if (prev_cp) {
            auto prev_kind = to_kind(prev_cp.value());
            auto kind = to_kind(cp);
            if ((prev_kind != kind && prev_kind != CharKind::kWhitespace) || cp == '\n') {
                break;
            }
        }
        prev_offset = offset;
        prev_cp = cp;
    }
    return prev_offset;
}

std::pair<size_t, size_t> surroundingWord(const base::PieceTree& tree, size_t offset) {
    auto walker = base::TreeWalker{&tree, offset};
    auto reverse_walker = base::ReverseTreeWalker{&tree, offset};

    // TODO: Implement peek so we don't need to reset the walkers below.
    // `std::max` just means we prioritize words, then punctuation, then whitespace.
    auto word_kind =
        std::max(to_kind(walker.next_codepoint()), to_kind(reverse_walker.next_codepoint()));

    size_t start = offset;
    size_t end = offset;
    // TODO: Implement peek so we don't need to reset the walker here.
    reverse_walker.seek(offset);
    while (!reverse_walker.exhausted()) {
        int32_t cp = reverse_walker.next_codepoint();
        size_t offset = reverse_walker.offset();

        auto kind = to_kind(cp);
        if (kind == word_kind && cp != '\n') {
            start = offset;
        } else {
            break;
        }
    }
    // TODO: Implement peek so we don't need to reset the walker here.
    walker.seek(offset);
    while (!walker.exhausted()) {
        int32_t cp = walker.next_codepoint();
        size_t offset = walker.offset();

        auto kind = to_kind(cp);
        if (kind == word_kind && cp != '\n') {
            end = offset;
        } else {
            break;
        }
    }
    return {start, end};
}

bool isInsideWord(const base::PieceTree& tree, size_t offset) {
    auto walker = base::TreeWalker{&tree, offset};
    auto reverse_walker = base::ReverseTreeWalker{&tree, offset};
    auto prev_kind = to_kind(reverse_walker.next_codepoint());
    auto next_kind = to_kind(walker.next_codepoint());
    return prev_kind == CharKind::kWord && next_kind == CharKind::kWord;
}

namespace {

constexpr CharKind to_kind(int32_t codepoint) {
    if (una::codepoint::is_whitespace(codepoint)) {
        return CharKind::kWhitespace;
    } else if (una::codepoint::is_alphanumeric(codepoint) || codepoint == '_') {
        return CharKind::kWord;
    } else {
        return CharKind::kPunctuation;
    }
}

}  // namespace

}  // namespace movement
}  // namespace gui
