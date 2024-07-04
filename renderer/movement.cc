#include "movement.h"

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

    caret.column = this->closestBoundaryForX(buffer, caret.line, mouse.x);
    caret.byte = buffer.byteOfLine(caret.line) + caret.column;
}

// TODO: Rewrite this so this operates on an already shaped line.
//       We should remove any glyph cache/font rasterization from this method.
size_t Movement::closestBoundaryForX(const base::Buffer& buffer, size_t line_index, int x) {
    std::string line_str = buffer.getLineContent(line_index);

    size_t line_offset = 0;
    int total_advance = 0;
    for (int offset : buffer.getUtf8Offsets(line_index)) {
        std::string_view key = std::string_view(line_str).substr(line_offset, offset);
        GlyphCache::Glyph& glyph = main_glyph_cache.getGlyph(key);

        int glyph_center = total_advance + glyph.advance / 2;
        if (glyph_center >= x) {
            return line_offset;
        }

        total_advance += glyph.advance;

        line_offset += offset;
    }
    return line_offset;
}

}
