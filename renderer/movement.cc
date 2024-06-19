#include "movement.h"

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace renderer {

Movement::Movement(GlyphCache& main_glyph_cache) : main_glyph_cache(main_glyph_cache) {}

void Movement::setCaretInfo(const base::Buffer& buffer, const Point& mouse, CaretInfo& caret) {
    caret.line = mouse.y / main_glyph_cache.lineHeight();
    if (caret.line > buffer.lineCount() - 1) {
        caret.line = buffer.lineCount() - 1;
    }
    if (mouse.y < 0) {
        caret.line = 0;
    }

    std::string start_line_str = buffer.getLineContent(caret.line);

    caret.column = this->closestBoundaryForX(start_line_str, mouse.x);
    caret.byte = buffer.byteOfLine(caret.line) + caret.column;
}

// TODO: Rewrite this so this operates on an already shaped line.
//       We should remove any glyph cache/font rasterization from this method.
size_t Movement::closestBoundaryForX(std::string_view line_str, int x) {
    size_t offset;
    size_t ret;
    int total_advance = 0;
    for (offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);
        std::string_view key = line_str.substr(offset, ret);
        GlyphCache::Glyph& glyph = main_glyph_cache.getGlyph(key);

        int glyph_center = total_advance + glyph.advance / 2;
        if (glyph_center >= x) {
            return offset;
        }

        total_advance += glyph.advance;
    }
    return offset;
}

}
