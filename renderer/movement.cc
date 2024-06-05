#include "movement.h"
#include "uni_algo/prop.h"

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace renderer {

Movement::Movement(GlyphCache& main_glyph_cache) : main_glyph_cache(main_glyph_cache) {}

void Movement::setCaretInfo(base::Buffer& buffer, Point& mouse, CaretInfo& caret) {
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

void Movement::moveCaretForwardChar(base::Buffer& buffer, CaretInfo& caret) {
    std::string line_str = buffer.getLineContent(caret.line);

    size_t ret = grapheme_next_character_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
    if (ret > 0) {
        caret.byte += ret;
        caret.column += ret;
    }
}

void Movement::moveCaretBackwardChar(base::Buffer& buffer, CaretInfo& caret) {
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

void Movement::moveCaretForwardWord(base::Buffer& buffer, CaretInfo& caret) {
    std::string line_str = buffer.getLineContent(caret.line);

    CharKind prev_kind = CharKind::kNone;
    size_t ret = 0;
    for (size_t offset = caret.column; offset < line_str.length(); offset += ret) {
        uint_least32_t codepoint = 0;

        ret = grapheme_decode_utf8(&line_str[0] + offset, SIZE_MAX, &codepoint);
        if (ret == 0) [[unlikely]] {
            break;
        }

        CharKind kind = classifyChar(codepoint);

        if (prev_kind != CharKind::kNone && kind != prev_kind) {
            break;
        }

        caret.byte += ret;
        caret.column += ret;

        prev_kind = kind;
    }
}

void Movement::moveCaretBackwardWord(base::Buffer& buffer, CaretInfo& caret) {
    std::string line_str = buffer.getLineContent(caret.line);

    std::vector<CharKind> kinds;
    std::vector<size_t> offsets;

    size_t ret = 0;
    for (size_t offset = 0; offset < caret.column; offset += ret) {
        uint_least32_t codepoint = 0;

        ret = grapheme_decode_utf8(&line_str[0] + offset, SIZE_MAX, &codepoint);
        if (ret == 0) [[unlikely]] {
            break;
        }

        CharKind kind = classifyChar(codepoint);
        kinds.push_back(std::move(kind));
        offsets.push_back(offset);
    }

    // Prevent overflow on `kinds.size() - 1` statement.
    if (kinds.empty()) {
        return;
    }

    size_t new_column = 0;
    for (size_t i = kinds.size() - 1; i >= 1; i--) {
        if (kinds[i] != kinds[i - 1]) {
            new_column = offsets[i];
            break;
        }
    }
    caret.byte -= caret.column - new_column;
    caret.column = new_column;
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

constexpr Movement::CharKind Movement::classifyChar(uint_least32_t codepoint) {
    if (una::codepoint::is_alphanumeric(codepoint) || codepoint == U'_') {
        return CharKind::kAlphanumeric;
    }
    if (una::codepoint::is_whitespace(codepoint)) {
        return CharKind::kWhitespace;
    }
    return CharKind::kOther;
}

}
