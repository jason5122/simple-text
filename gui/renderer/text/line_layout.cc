#include "line_layout.h"

// TODO: Debug; remove this.
#include "util/profile_util.h"
#include <format>
#include <iostream>

namespace gui {

void LineLayout::layout(const base::Buffer& buffer, GlyphCache& main_glyph_cache) {
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
        });

        total_advance += glyph.advance;

        if (is_newline) {
            // Cache byte offsets of newlines.
            newline_offsets.emplace_back(tokens.size());

            longest_line_x = std::max(total_advance, longest_line_x);
            total_advance = 0;
        }
    }

    std::cerr << std::format("tokens.size() = {}, newline_offsets.size() = {}\n", tokens.size(),
                             newline_offsets.size());
    std::cerr << std::format("longest_line_x = {}\n", longest_line_x);
    {
        PROFILE_BLOCK("LineLayout: iterate through tokens");
        int lol = 0;
        for (auto it = line(0); it != line(20); it++) {
            const auto& token = *it;
            lol += token.total_advance;
        }
        std::cerr << std::format("lol = {}\n", lol);
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

}
