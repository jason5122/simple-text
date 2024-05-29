#pragma once

#include "base/buffer.h"
#include "renderer/glyph_cache.h"
#include "renderer/types.h"

namespace renderer {

class Movement {
public:
    Movement(GlyphCache& main_glyph_cache);
    void setCaretInfo(base::Buffer& buffer, Point& mouse, CaretInfo& caret);
    void moveCaretForwardChar(base::Buffer& buffer, CaretInfo& caret);
    void moveCaretBackwardChar(base::Buffer& buffer, CaretInfo& caret);
    void moveCaretForwardWord(base::Buffer& buffer, CaretInfo& caret);
    void moveCaretBackwardWord(base::Buffer& buffer, CaretInfo& caret);

private:
    GlyphCache& main_glyph_cache;

    enum class CharKind {
        kNone,
        kAlphanumeric,
        kWhitespace,
        kOther,
        // TODO: Maybe add more kinds.
    };

    size_t closestBoundaryForX(std::string_view line_str, int x);
    constexpr CharKind classifyChar(uint_least32_t codepoint);
};

}
