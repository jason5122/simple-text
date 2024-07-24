#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "line_layout.h"
#include "util/profile_util.h"
#include <algorithm>
#include <numeric>

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

namespace gui {

LineLayout::LineLayout(const base::PieceTable& table, const base::Buffer& buffer) {
    GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();
    reflow(table, buffer, main_glyph_cache);
}

font::FontRasterizer::LineLayout& LineLayout::getLineLayout(size_t line) {
    return line_layouts[line];
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

LineLayout::Iterator LineLayout::iteratorFromPoint(size_t line, const Point& point) {
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

LineLayout::Iterator LineLayout::moveByLines(bool forward, Iterator caret, int x) {
    size_t line = (*caret).line;

    if (forward) {
        line = base::add_sat(line, 1UL);
    } else {
        // Edge case.
        // TODO: See if we can handle this cleaner.
        if (line == 0) {
            return begin();
        }

        line = base::sub_sat(line, 1UL);
    }
    line = std::clamp(line, 0UL, newline_offsets.size());

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

void LineLayout::reflow(const base::PieceTable& table,
                        const base::Buffer& buffer,
                        GlyphCache& main_glyph_cache) {
    {
        PROFILE_BLOCK("Core Text reflow");
        size_t lines = table.newlineCount() + 1;
        for (size_t i = 0; i < lines; i++) {
            std::string line = table.line(i);
            auto layout = main_glyph_cache.rasterizer().layoutLine(line);
            line_layouts.push_back(std::move(layout));
        }
    }
}

size_t LineLayout::iteratorIndex(Iterator it) {
    size_t index = std::distance(begin(), it);
    return std::clamp(index, 0UL, tokens.size());
}

LineLayout::Iterator LineLayout::getIterator(size_t index) {
    return std::min(begin() + index, std::prev(end()));
}

}
