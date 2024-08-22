#pragma once

#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/opengl_types.h"
#include <unordered_map>
#include <vector>

namespace gui {

class GlyphCache {
public:
    GlyphCache();

    struct Glyph {
        GLuint tex_id;
        Vec4 glyph;
        Vec4 uv;
        int32_t advance;
        bool colored;
        size_t page;
    };

    Glyph& getGlyph(size_t layout_font_id,
                    size_t font_id,
                    uint32_t glyph_id,
                    const font::FontRasterizer& font_rasterizer,
                    float subpixel_variant_x);

    void setMainFontId(size_t font_id);
    void setUIFontId(size_t font_id);
    size_t mainFontId() const;
    size_t uiFontId() const;
    const std::vector<Atlas>& atlasPages() const;

private:
    size_t main_font_id;
    size_t ui_font_id;

    std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    // [layout_font_id, run_font_id, glyph_id] -> Glyph
    // std::vector<std::vector<std::unordered_map<uint32_t, Glyph>>> cache;
    // std::vector<std::vector<std::unordered_map<uint32_t, std::unordered_map<int, Glyph>>>>
    // cache;
    std::vector<Glyph> cache;

    Glyph loadGlyph(const font::RasterizedGlyph& rglyph);
};

}
