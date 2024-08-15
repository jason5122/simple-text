#pragma once

#include "font/font_rasterizer.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/opengl_types.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace gui {

class GlyphCache {
public:
    GlyphCache(const std::string& main_font_name_utf8,
               int main_font_size,
               const std::string& ui_font_name_utf8,
               int ui_font_size);

    struct Glyph {
        GLuint tex_id;
        Vec4 glyph;
        Vec4 uv;
        int32_t advance;
        bool colored;
        size_t page;
    };

    Glyph& getGlyph(size_t layout_font_id, size_t font_id, uint32_t glyph_id);

    size_t mainFontId() const;
    size_t uiFontId() const;
    const font::FontRasterizer& fontRasterizer() const;
    const std::vector<Atlas>& atlasPages() const;

private:
    size_t main_font_id;
    size_t ui_font_id;
    font::FontRasterizer font_rasterizer;

    std::vector<Atlas> atlas_pages;
    size_t current_page = 0;

    // [layout_font_id, run_font_id, glyph_id] -> Glyph
    std::vector<std::vector<std::unordered_map<uint32_t, Glyph>>> cache;

    Glyph loadGlyph(const font::RasterizedGlyph& rglyph);
};

}
