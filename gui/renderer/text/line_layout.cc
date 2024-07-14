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

std::vector<LineLayout::Token>::const_iterator LineLayout::begin() const {
    return tokens.begin();
}

std::vector<LineLayout::Token>::const_iterator LineLayout::end() const {
    return tokens.end();
}

std::vector<LineLayout::Token>::const_iterator LineLayout::line(size_t line) const {
    if (line >= newline_offsets.size()) {
        return end();
    } else {
        return tokens.begin() + newline_offsets.at(line);
    }
}

std::vector<LineLayout::Token>::const_iterator LineLayout::iteratorFromPoint(const Point& point) {
    int y = std::max(point.y, 0);
    size_t line_index = y / main_glyph_cache.lineHeight();
    for (auto it = line(line_index); it != line(line_index + 1); it++) {
        const auto& token = *it;
        const auto& next_token = *std::next(it);

        int glyph_center = std::midpoint(token.total_advance, next_token.total_advance);
        if (glyph_center >= point.x) {
            return it;
        }
    }
    return std::prev(line(line_index + 1));
}

}
