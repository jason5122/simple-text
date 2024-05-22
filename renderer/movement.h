#pragma once

#include "base/buffer.h"
#include "renderer/glyph_cache.h"
#include "renderer/types.h"

namespace renderer {
class Movement {
public:
    Movement(GlyphCache& main_glyph_cache);
    void setCaretInfo(Buffer& buffer, Point& mouse, CaretInfo& caret);
    void moveCaretForwardChar(Buffer& buffer, CaretInfo& caret);
    void moveCaretForwardWord(Buffer& buffer, CaretInfo& caret);

private:
    GlyphCache& main_glyph_cache;

    std::pair<int, size_t> closestBoundaryForX(std::string_view line_str, int x);
};
}
