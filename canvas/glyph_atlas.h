#pragma once

#include "gfx/device.h"
#include "gfx/texture.h"
#include "text/font_rasterizer.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace canvas {

// A cached, packed glyph in the atlas. `empty` glyphs (e.g. spaces) have no pixels and no page.
struct GlyphEntry {
    bool empty = false;
    size_t page = 0;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    int left = 0, top = 0, width = 0, height = 0;
    bool colored = false;
};

// Caches rasterized glyphs in GPU texture pages, keyed by (font_id, glyph_id). Glyphs are packed
// with a simple shelf packer; pages are allocated on demand and never evicted (sufficient for the
// current working set). Glyph bitmaps are premultiplied BGRA, uploaded via Texture::update_region.
class GlyphAtlas {
public:
    explicit GlyphAtlas(gfx::Device& device) : device_(device) {}

    const GlyphEntry& get(text::FontId font_id, uint32_t glyph_id);
    gfx::Texture& page(size_t index) { return *pages_[index]; }

private:
    struct Key {
        text::FontId font_id;
        uint32_t glyph_id;
        bool operator==(const Key&) const = default;
    };
    struct KeyHash {
        size_t operator()(const Key& k) const {
            return std::hash<text::FontId>{}(k.font_id) * 1099511628211u ^
                   std::hash<uint32_t>{}(k.glyph_id);
        }
    };

    void add_page();
    GlyphEntry pack(const text::RasterizedGlyph& glyph);

    static constexpr int kPageSize = 1024;

    gfx::Device& device_;
    std::vector<std::unique_ptr<gfx::Texture>> pages_;
    std::unordered_map<Key, GlyphEntry, KeyHash> cache_;

    int cursor_x_ = 0;
    int cursor_y_ = 0;
    int row_height_ = 0;
};

}  // namespace canvas
