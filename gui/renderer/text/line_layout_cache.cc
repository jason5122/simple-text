#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "line_layout_cache.h"
#include <algorithm>
#include <numeric>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <format>
#include <iostream>

namespace gui {

LineLayoutCache::LineLayoutCache(const base::PieceTable& table) {
    GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();
    for (size_t i = 0; i < table.lineCount(); ++i) {
        std::string line = table.line(i);
        auto layout = main_glyph_cache.rasterizer().layoutLine(line);
        line_layouts.push_back(std::move(layout));

        max_width = std::max(layout.width, max_width);
    }
}

const font::FontRasterizer::LineLayout& LineLayoutCache::getLineLayout(size_t line) const {
    return line_layouts[line];
}

void LineLayoutCache::reflow(const base::PieceTable& table, size_t line) {
    GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();

    std::string line_str = table.line(line);
    auto layout = main_glyph_cache.rasterizer().layoutLine(line_str);
    line_layouts[line] = std::move(layout);

    // TODO: Debug use; remove this.
    // Reflow all lines.
    // line_layouts.clear();
    // for (size_t i = 0; i < table.lineCount(); ++i) {
    //     std::string line = table.line(i);
    //     auto layout = main_glyph_cache.rasterizer().layoutLine(line);
    //     line_layouts.push_back(std::move(layout));
    // }
}

int LineLayoutCache::maxWidth() const {
    return max_width;
}

// LineLayoutCache::Iterator LineLayoutCache::iteratorFromPoint(size_t line, const Point& point) {
//     for (auto it = getLine(line); it != getLine(line + 1); ++it) {
//         const auto& token = *it;
//         const auto& next_token = *std::next(it);

//         int glyph_center = std::midpoint(token.total_advance, next_token.total_advance);
//         if (glyph_center >= point.x) {
//             return it;
//         }
//     }
//     return std::prev(getLine(line + 1));
// }

// LineLayoutCache::Iterator LineLayoutCache::moveByCharacters(bool forward, Iterator caret) {
//     if (forward && caret != std::prev(end())) {
//         return std::next(caret);
//     }
//     if (!forward && caret != begin()) {
//         return std::prev(caret);
//     }
//     return caret;
// }

// LineLayoutCache::Iterator LineLayoutCache::moveByLines(bool forward, Iterator caret, int x) {
//     size_t line = (*caret).line;

//     if (forward) {
//         line = base::add_sat(line, 1UL);
//     } else {
//         // Edge case.
//         // TODO: See if we can handle this cleaner.
//         if (line == 0) {
//             return begin();
//         }

//         line = base::sub_sat(line, 1UL);
//     }
//     line = std::clamp(line, 0UL, newline_offsets.size());

//     for (auto it = getLine(line); it != getLine(line + 1); ++it) {
//         const auto& token = *it;
//         const auto& next_token = *std::next(it);

//         int glyph_center = std::midpoint(token.total_advance, next_token.total_advance);
//         if (glyph_center >= x) {
//             return it;
//         }
//     }
//     return std::prev(getLine(line + 1));
// }

}
