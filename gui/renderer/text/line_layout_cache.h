#pragma once

#include "base/buffer/piece_table.h"
#include "gui/renderer/text/glyph_cache.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

// TODO: Use a better name than this maybe.
class LineLayoutCache {
public:
    LineLayoutCache(const base::PieceTable& table);

    const font::FontRasterizer::LineLayout& getLineLayout(size_t line) const;
    void reflow(const base::PieceTable& table, size_t line);

    struct Token {
        size_t line;
        int total_advance;
        int advance;
        GlyphCache::Glyph& glyph;
        // size_t byte_offset;
    };

    using Iterator = std::vector<Token>::const_iterator;

    Iterator begin() const;
    Iterator end() const;
    Iterator getLine(int line) const;
    Iterator iteratorFromPoint(size_t line, const Point& point);
    Iterator moveByCharacters(bool forward, Iterator caret);
    Iterator moveByLines(bool forward, Iterator caret, int x);

    // TODO: Clean this up.
    size_t iteratorIndex(Iterator it);
    Iterator getIterator(size_t index);

    // TODO: Use a data structure (priority queue) for efficient updating.
    // TODO: Make this private.
    int longest_line_x = 10000;

private:
    std::vector<font::FontRasterizer::LineLayout> line_layouts;
};

}
