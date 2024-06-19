#pragma once

#include "base/buffer.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/types.h"

namespace renderer {

class Movement {
public:
    Movement(GlyphCache& main_glyph_cache);
    void setCaretInfo(const base::Buffer& buffer, const Point& mouse, CaretInfo& caret);

private:
    GlyphCache& main_glyph_cache;

    size_t closestBoundaryForX(std::string_view line_str, int x);
};

}
