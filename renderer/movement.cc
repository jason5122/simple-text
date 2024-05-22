#include "movement.h"

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace renderer {
Movement::Movement(GlyphCache& main_glyph_cache) : main_glyph_cache(main_glyph_cache) {}

void Movement::setCaretInfo(Buffer& buffer, Point& mouse, CaretInfo& caret) {
    int x;
    size_t offset;

    caret.line = mouse.y / main_glyph_cache.lineHeight();
    if (caret.line > buffer.lineCount() - 1) {
        caret.line = buffer.lineCount() - 1;
    }

    std::string start_line_str = buffer.getLineContent(caret.line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, mouse.x);
    caret.column = offset;

    caret.byte = buffer.byteOfLine(caret.line) + caret.column;
}

void Movement::moveCaretForwardChar(Buffer& buffer, CaretInfo& caret) {
    std::string line_str = buffer.getLineContent(caret.line);

    size_t ret = grapheme_next_character_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
    if (ret > 0) {
        caret.byte += ret;
        caret.column += ret;
    }
}

void Movement::moveCaretBackwardChar(Buffer& buffer, CaretInfo& caret) {
    std::string line_str = buffer.getLineContent(caret.line);

    size_t new_column = 0;
    size_t ret = 0;
    for (new_column = 0; new_column < caret.column; new_column += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + new_column, SIZE_MAX);
    }
    new_column -= ret;

    caret.byte -= caret.column - new_column;
    caret.column = new_column;
}

// TODO: Do we really need a Unicode-accurate version of this? Sublime Text doesn't seem to follow
// Unicode word boundaries the way libgrapheme does.
void Movement::moveCaretForwardWord(Buffer& buffer, CaretInfo& caret) {
    std::string line_str = buffer.getLineContent(caret.line);

    size_t word_offset = grapheme_next_word_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
    if (word_offset > 0) {
        int total_advance = 0;
        size_t ret;
        for (size_t offset = caret.column; offset < caret.column + word_offset; offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
            std::string_view key = std::string_view(line_str).substr(offset, ret);
            AtlasGlyph& glyph = main_glyph_cache.getGlyph(key);
            total_advance += glyph.advance;
        }

        caret.byte += word_offset;
        caret.column += word_offset;
    }
}

// TODO: Rewrite this so this operates on an already shaped line.
//       We should remove any glyph cache/font rasterization from this method.
std::pair<int, size_t> Movement::closestBoundaryForX(std::string_view line_str, int x) {
    size_t offset;
    size_t ret;
    int total_advance = 0;
    for (offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);
        std::string_view key = line_str.substr(offset, ret);
        AtlasGlyph& glyph = main_glyph_cache.getGlyph(key);

        int glyph_center = total_advance + glyph.advance / 2;
        if (glyph_center >= x) {
            return {total_advance, offset};
        }

        total_advance += glyph.advance;
    }
    return {total_advance, offset};
}
}
