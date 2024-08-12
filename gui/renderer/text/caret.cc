#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "caret.h"
#include <cassert>
#include <numeric>

namespace gui {

void Caret::moveToPoint(const font::LineLayout& layout, size_t line, const Point& point) {
    for (size_t i = 0; i < layout.runs.size(); i++) {
        const auto& run = layout.runs[i];

        size_t last_run = base::sub_sat(layout.runs.size(), 1_Z);
        size_t last_run_glyph = base::sub_sat(run.glyphs.size(), 1_Z);

        for (size_t j = 0; j < run.glyphs.size(); j++) {
            const auto& glyph = run.glyphs[j];

            int glyph_x = glyph.position.x;
            int glyph_center = std::midpoint(glyph_x, glyph_x + glyph.advance.x);
            if (glyph_center >= point.x) {
                this->line = line;
                this->x = glyph_x;
                this->index = glyph.index;

                this->run_index = i;
                this->run_glyph_index = j;
                return;
            }
        }
    }
    this->line = line;
    this->x = layout.width;
    this->index = layout.length;

    // int width = 0;
    // size_t index = 0;

    // if (!layout.runs.empty()) {
    //     const auto& last_glyph = layout.runs.back().glyphs.back();
    //     width = last_glyph.position.x;
    //     index = last_glyph.index;
    // }

    // size_t last_layout_line = base::sub_sat(line_layouts.size(), 1_Z);
    // if (line == last_layout_line) {
    //     width = layout.width;
    //     index = layout.length;
    // }

    // this->line = line;
    // this->x = width;
    // this->index = index;
}

void Caret::moveByCharacters(const font::LineLayout& layout, bool forward) {
    // assert(run_index < layout.runs.size());

    // auto glyphs = layout.runs[run_index].glyphs;
    // assert(run_glyph_index < glyphs.size());

    // if (forward) {
    //     // Move to next glyph in run. If we reach the end, move to the next run.
    //     // If there are no more runs, remain at the last glyph of the last run.
    //     if (run_glyph_index + 1 < glyphs.size()) {
    //         ++run_glyph_index;
    //     } else {
    //         if (run_index + 1 < layout.runs.size()) {
    //             ++run_index;
    //             run_glyph_index = 0;
    //         }
    //     }

    //     auto glyph = layout.runs[run_index].glyphs[run_glyph_index];
    //     x = glyph.position.x;
    //     index = glyph.index;
    // }
}

static_assert(Caret{0, 0} < Caret{0, 1});
static_assert(Caret{0, 1} < Caret{1, 0});
static_assert(Caret{1, 0} < Caret{1, 1});
static_assert(!(Caret{1, 0} < Caret{1, 0}));

static_assert(Caret{0, 1} > Caret{0, 0});
static_assert(Caret{1, 0} > Caret{0, 1});
static_assert(Caret{1, 1} > Caret{1, 0});
static_assert(!(Caret{1, 0} > Caret{1, 0}));

}
