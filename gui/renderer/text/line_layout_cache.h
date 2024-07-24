#pragma once

#include "base/buffer/piece_table.h"
#include "font/font_rasterizer.h"
#include <vector>

namespace gui {

// TODO: Use a better name than this maybe.
class LineLayoutCache {
public:
    LineLayoutCache(const base::PieceTable& table);

    const font::FontRasterizer::LineLayout& getLineLayout(size_t line) const;
    void reflow(const base::PieceTable& table, size_t line);
    int maxWidth() const;

private:
    std::vector<font::FontRasterizer::LineLayout> line_layouts;

    // TODO: Use a data structure (priority queue) for efficient updating.
    int max_width = 0;
};

}
