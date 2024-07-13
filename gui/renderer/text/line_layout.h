#pragma once

#include "base/buffer.h"
#include "gui/renderer/text/glyph_cache.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

// TODO: Use a better name than this maybe.
class LineLayout {
public:
    struct Token {
        size_t line;
        int total_advance;
        int advance;
        GlyphCache::Glyph& glyph;
    };

    std::vector<Token>::const_iterator begin() const;
    std::vector<Token>::const_iterator end() const;
    std::vector<Token>::const_iterator line(size_t line) const;

    void layout(const base::Buffer& buffer, GlyphCache& main_glyph_cache);
    std::vector<Token>::const_iterator iteratorFromPoint(const base::Buffer& buffer,
                                                         GlyphCache& main_glyph_cache,
                                                         const Point& point);

    // TODO: Use a data structure (priority queue) for efficient updating.
    // TODO: Make this private.
    int longest_line_x = 0;

private:
    // TODO: Consider renaming "Token" and "tokens".
    std::vector<Token> tokens;
    std::vector<size_t> newline_offsets;
};

}
