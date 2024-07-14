#include "line_layout.h"
#include <algorithm>
#include <numeric>

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

namespace gui {

LineLayout::LineLayout(const base::Buffer& buffer, GlyphCache& main_glyph_cache)
    : buffer{buffer}, main_glyph_cache{main_glyph_cache} {
    // Cache byte offsets of newlines.
    newline_offsets.emplace_back(tokens.size());

    int total_advance = 0;
    for (auto it = buffer.begin(); it != buffer.end(); it++) {
        const auto& ch = *it;
        const bool is_newline = ch.line != (*std::next(it)).line;

        // Render newline characters as spaces, since DirectWrite and Pango don't seem to
        // support rendering "\n".
        std::string_view key = is_newline ? " " : ch.str;
        GlyphCache::Glyph& glyph = main_glyph_cache.getGlyph(key);

        tokens.emplace_back(Token{
            .line = ch.line,
            .total_advance = total_advance,
            .advance = glyph.advance,
            .glyph = glyph,
        });

        total_advance += glyph.advance;

        if (is_newline) {
            // Cache byte offsets of newlines.
            newline_offsets.emplace_back(tokens.size());

            longest_line_x = std::max(total_advance, longest_line_x);
            total_advance = 0;
        }
    }
}

LineLayout::Iterator LineLayout::begin() const {
    return tokens.begin();
}

LineLayout::Iterator LineLayout::end() const {
    return tokens.end();
}

LineLayout::Iterator LineLayout::getLine(int line) const {
    if (line >= newline_offsets.size()) {
        return end();
    } else {
        return tokens.begin() + newline_offsets.at(line);
    }
}

LineLayout::Iterator LineLayout::iteratorFromPoint(const Point& point) {
    int y = std::max(point.y, 0);
    size_t line = y / main_glyph_cache.lineHeight();
    for (auto it = getLine(line); it != getLine(line + 1); it++) {
        const auto& token = *it;
        const auto& next_token = *std::next(it);

        int glyph_center = std::midpoint(token.total_advance, next_token.total_advance);
        if (glyph_center >= point.x) {
            return it;
        }
    }
    return std::prev(getLine(line + 1));
}

LineLayout::Iterator LineLayout::moveByCharacters(bool forward, Iterator caret) {
    if (forward && caret != std::prev(end())) {
        return std::next(caret);
    }
    if (!forward && caret != begin()) {
        return std::prev(caret);
    }
    return caret;
}

LineLayout::Iterator LineLayout::moveByLines(bool forward, Iterator caret) {
    size_t line = (*caret).line;
    int x = (*caret).total_advance;

    if (forward) {
        if (line < newline_offsets.size()) line++;  // Saturating addition;
    } else {
        if (line > 0) line--;  // Saturating subtraction.
    }

    // Edge case.
    // TODO: See if we can handle this cleaner.
    if (line == 0 && !forward) {
        return begin();
    }

    for (auto it = getLine(line); it != getLine(line + 1); it++) {
        const auto& token = *it;
        const auto& next_token = *std::next(it);

        int glyph_center = std::midpoint(token.total_advance, next_token.total_advance);
        if (glyph_center >= x) {
            return it;
        }
    }
    return std::prev(getLine(line + 1));
}

}
