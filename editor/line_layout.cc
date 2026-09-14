#include "base/check.h"
#include "editor/line_layout.h"
#include "fx/fx.h"
#include <cmath>
#include <memory>

namespace editor {

LineLayout layout_line(size_t font_id, fx_font& font, float scale, std::string_view str8) {
    DCHECK_EQ(str8.find('\n'), std::string_view::npos);

    LineLayout layout = {
        .font_id = font_id,
        .width = 0,
        .length = str8.length(),
    };

    std::unique_ptr<fx_layout> shaped = font.shape(str8);
    if (!shaped) {
        return layout;
    }

    const auto to_device = [scale](float logical) {
        return static_cast<int>(std::lround(logical * scale));
    };

    // fx reports where each glyph's ink goes, not where its pen is. A backend that snaps
    // advances to whole pixels (Core Text does, for monospace fonts at small sizes) folds the
    // glyph's own rounding delta into its x_offset so the ink lands where Sublime Text draws it,
    // and every glyph of the run shares that delta. Carets, selections and hit testing need the
    // pen grid instead, and `advance` is already the exact end of the last cell. The first
    // glyph's pen is always 0, so its x_offset is exactly the delta to remove.
    const float pen_shift = shaped->glyphs.empty() ? 0.0f : shaped->glyphs.front().x_offset;
    const auto pen_of = [&](const fx_glyph& glyph) {
        return to_device(glyph.x_offset - pen_shift);
    };

    layout.width = to_device(shaped->advance);
    layout.glyphs.reserve(shaped->glyphs.size());
    for (size_t i = 0; i < shaped->glyphs.size(); ++i) {
        const fx_glyph& glyph = shaped->glyphs[i];
        // fx does not report per-glyph advances; derive them from consecutive pens so that each
        // glyph's advance ends exactly where the next glyph begins.
        const int x = pen_of(glyph);
        const int next_x =
            i + 1 < shaped->glyphs.size() ? pen_of(shaped->glyphs[i + 1]) : layout.width;
        layout.glyphs.push_back({
            .glyph_id = glyph.id,
            .x = x,
            .y = to_device(glyph.y_offset),
            .advance = next_x - x,
            .index = glyph.cluster,
        });
    }
    return layout;
}

}  // namespace editor
