#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "line_layout_cache.h"
#include <algorithm>
#include <cassert>
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
    assert(line < line_layouts.size());
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

void LineLayoutCache::moveToPoint(size_t line, const Point& point, Caret& caret) {
    if (line >= line_layouts.size()) {
        if (!line_layouts.empty()) {
            caret.line = base::sub_sat(line_layouts.size(), 1UL);
            const auto& last_glyph = line_layouts.back().runs.back().glyphs.back();
            caret.x = line_layouts.back().width;
            caret.index = last_glyph.index;
        }
        return;
    }

    caret.line = line;

    auto layout = getLineLayout(line);
    for (const auto& run : layout.runs) {
        for (const auto& glyph : run.glyphs) {
            int glyph_x = glyph.position.x;
            int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
            if (glyph_center >= point.x) {
                caret.x = glyph_x;
                caret.index = glyph.index;
                return;
            }
        }
    }
    caret.x = layout.width;
    if (!layout.runs.empty()) {
        caret.index = layout.runs.back().glyphs.back().index;
    }
}

void LineLayoutCache::moveByCharacters(bool forward, Caret& caret) {
    auto layout = getLineLayout(caret.line);
    assert(caret.run_index < layout.runs.size());

    auto glyphs = layout.runs[caret.run_index].glyphs;
    assert(caret.run_glyph_index < glyphs.size());

    if (forward) {
        // Move to next glyph in run. If we reach the end, move to the next run.
        // If there are no more runs, remain at the last glyph of the last run.
        if (caret.run_glyph_index + 1 < glyphs.size()) {
            ++caret.run_glyph_index;
        } else {
            if (caret.run_index + 1 < layout.runs.size()) {
                ++caret.run_index;
                caret.run_glyph_index = 0;
            }
        }

        auto glyph = layout.runs[caret.run_index].glyphs[caret.run_glyph_index];
        caret.x = glyph.position.x;
        caret.index = glyph.index;
    }
}

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
