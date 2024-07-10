#pragma once

#include "base/buffer.h"
#include "gui/renderer/text/glyph_cache.h"
#include "gui/renderer/types.h"

namespace gui {

class Movement {
public:
    Movement(GlyphCache& main_glyph_cache);
    void setCaretInfo(const base::Buffer& buffer, const Point& mouse, CaretInfo& caret);

private:
    GlyphCache& main_glyph_cache;

    size_t closestBoundaryForX(const base::Buffer& buffer, size_t line_index, int x);
};

}
