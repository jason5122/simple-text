#pragma once

#include "font/rasterizer.h"
#include "renderer/image/image_renderer.h"
#include "renderer/movement.h"
#include "renderer/rect_renderer.h"
#include "renderer/selection_renderer.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/text/text_renderer.h"
#include "simple_text/editor_tab.h"

namespace renderer {

class Renderer {
public:
    Renderer(font::FontRasterizer& main_font_rasterizer, font::FontRasterizer& ui_font_rasterizer);
    void setup();
    // TODO: Split this up into smaller methods.
    void render(Size& size, config::ColorScheme& color_scheme,
                std::vector<std::unique_ptr<EditorTab>>& tabs, size_t tab_index);
    void toggleSideBar();
    Point translateMousePosition(int mouse_x, int mouse_y, EditorTab* tab);
    int lineHeight();

    // TODO: Make this private!
    Movement movement;

private:
    GlyphCache main_glyph_cache;
    GlyphCache ui_glyph_cache;
    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    // ImageRenderer image_renderer;
    // SelectionRenderer selection_renderer;

    static constexpr int kLineNumberOffset = 120;

    bool side_bar_visible = true;
    renderer::Point editor_offset{
        .x = 200 * 2,
        .y = 30 * 2,
    };
};

}
