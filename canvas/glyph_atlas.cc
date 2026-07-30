#include "canvas/glyph_atlas.h"
#include <algorithm>

namespace canvas {

void GlyphAtlas::add_page() {
    pages_.push_back(device_.create_texture(kPageSize, kPageSize, gfx::TextureFormat::kBGRA8));
    cursor_x_ = 0;
    cursor_y_ = 0;
    row_height_ = 0;
}

const GlyphEntry& GlyphAtlas::get(text::FontId font_id, uint32_t glyph_id) {
    const Key key{font_id, glyph_id};
    if (auto it = cache_.find(key); it != cache_.end()) return it->second;

    text::RasterizedGlyph glyph = text::FontRasterizer::instance().rasterize(font_id, glyph_id);
    auto [it, _] = cache_.emplace(key, pack(glyph));
    return it->second;
}

GlyphEntry GlyphAtlas::pack(const text::RasterizedGlyph& glyph) {
    GlyphEntry entry;
    entry.left = glyph.left;
    entry.top = glyph.top;
    entry.width = glyph.width;
    entry.height = glyph.height;
    entry.colored = glyph.colored;

    if (glyph.width <= 0 || glyph.height <= 0 || glyph.buffer.empty()) {
        entry.empty = true;
        return entry;
    }

    if (pages_.empty()) add_page();

    // Advance to the next shelf if the glyph doesn't fit in the current row; start a new page if it
    // doesn't fit vertically.
    if (cursor_x_ + glyph.width > kPageSize) {
        cursor_x_ = 0;
        cursor_y_ += row_height_;
        row_height_ = 0;
    }
    if (cursor_y_ + glyph.height > kPageSize) {
        add_page();
    }

    const int x = cursor_x_;
    const int y = cursor_y_;
    pages_.back()->update_region(x, y, glyph.width, glyph.height, gfx::TextureFormat::kBGRA8,
                                 glyph.buffer);

    cursor_x_ += glyph.width;
    row_height_ = std::max(row_height_, glyph.height);

    entry.page = pages_.size() - 1;
    entry.u0 = static_cast<float>(x) / kPageSize;
    entry.v0 = static_cast<float>(y) / kPageSize;
    entry.u1 = static_cast<float>(x + glyph.width) / kPageSize;
    entry.v1 = static_cast<float>(y + glyph.height) / kPageSize;
    return entry;
}

}  // namespace canvas
