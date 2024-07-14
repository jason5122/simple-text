#pragma once

#include "base/buffer.h"
#include "gui/renderer/text/glyph_cache.h"
#include "gui/renderer/types.h"
#include "gui/widget/types.h"
#include <vector>

namespace gui {

// TODO: Use a better name than this maybe.
class LineLayout {
public:
    LineLayout(const base::Buffer& buffer, GlyphCache& main_glyph_cache);

    struct Token {
        size_t line;
        int total_advance;
        int advance;
        GlyphCache::Glyph& glyph;
    };

    using Iterator = std::vector<Token>::const_iterator;

    Iterator begin() const;
    Iterator end() const;
    Iterator getLine(int line) const;
    Iterator iteratorFromPoint(const Point& point);
    Iterator moveByLines(bool forward, Iterator caret);

    // TODO: Use a data structure (priority queue) for efficient updating.
    // TODO: Make this private.
    int longest_line_x = 0;

private:
    const base::Buffer& buffer;
    GlyphCache& main_glyph_cache;

    // TODO: Consider renaming "Token" and "tokens".
    std::vector<Token> tokens;
    std::vector<size_t> newline_offsets;
};

}
