#pragma once

#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/opengl_types.h"

#include "third_party/hash_maps/robin_hood.h"

#include <vector>

namespace gui {

class GlyphCache {
public:
    GlyphCache();

    struct Glyph {
        Vec4 glyph;
        Vec4 uv;
        bool colored;
        size_t page;
    };

    const Glyph& getGlyph(size_t font_id, uint32_t glyph_id);

    const std::vector<Atlas>& atlasPages() const;

private:
    std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    // We use a node-based map since we need to keep references stable.
    std::vector<robin_hood::unordered_node_map<uint32_t, Glyph>> cache;

    Glyph insertIntoAtlas(const font::RasterizedGlyph& rglyph);
};

}  // namespace gui
