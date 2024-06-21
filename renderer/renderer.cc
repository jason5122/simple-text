#include "renderer.h"

namespace renderer {

Renderer::Renderer(font::FontRasterizer& main_font_rasterizer,
                   font::FontRasterizer& ui_font_rasterizer)
    : main_glyph_cache(main_font_rasterizer), ui_glyph_cache(ui_font_rasterizer),
      text_renderer(main_glyph_cache, ui_glyph_cache), movement(main_glyph_cache) {}

void Renderer::setup() {
    main_glyph_cache.setup();
    ui_glyph_cache.setup();

    text_renderer.setup();
    rect_renderer.setup();
    image_renderer.setup();
    selection_renderer.setup();
}

void Renderer::render(Size& size, config::ColorScheme& color_scheme,
                      std::vector<std::unique_ptr<EditorTab>>& tabs, size_t tab_index) {
    using Selection = renderer::SelectionRenderer::Selection;

    glViewport(0, 0, size.width, size.height);

    base::Rgb& background = color_scheme.background;
    GLfloat red = static_cast<float>(background.r) / 255.0f;
    GLfloat green = static_cast<float>(background.g) / 255.0f;
    GLfloat blue = static_cast<float>(background.b) / 255.0f;
    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    // Setup.
    std::vector<Selection> selections =
        text_renderer.getSelections(tab->buffer, tab->start_caret, tab->end_caret);

    selection_renderer.createInstances(size, tab->scroll, editor_offset, main_glyph_cache,
                                       selections, kLineNumberOffset);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    selection_renderer.render(0);
    selection_renderer.render(1);

    // Cleanup.
    selection_renderer.destroyInstances();
}

void Renderer::toggleSideBar() {
    if (side_bar_visible) {
        editor_offset.x = 0;
    } else {
        editor_offset.x = 200 * 2;
    }
    side_bar_visible = !side_bar_visible;
}

Point Renderer::translateMousePosition(int mouse_x, int mouse_y, EditorTab* tab) {
    return {
        .x = mouse_x - editor_offset.x - kLineNumberOffset + tab->scroll.x,
        .y = mouse_y - editor_offset.y + tab->scroll.y,
    };
}

int Renderer::lineHeight() {
    return main_glyph_cache.lineHeight();
}

}
