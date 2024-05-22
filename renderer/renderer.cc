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

    Rgb& background = color_scheme.background;
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

    // Render.
    int end_caret_x = -1;

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    selection_renderer.render(0);
    text_renderer.renderText(size, tab->scroll, tab->buffer, tab->highlighter, editor_offset,
                             tab->start_caret, tab->end_caret, tab->longest_line_x, color_scheme,
                             kLineNumberOffset, end_caret_x);
    selection_renderer.render(1);

    std::vector<int> tab_title_widths = text_renderer.getTabTitleWidths(tab->buffer, tabs);
    std::vector<int> tab_title_x_coords;
    std::vector<int> actual_tab_title_widths;

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, tab->scroll, tab->end_caret, end_caret_x,
                       main_glyph_cache.lineHeight(), tab->buffer.lineCount(), tab->longest_line_x,
                       editor_offset, ui_glyph_cache.lineHeight(), color_scheme, tab_index,
                       tab_title_widths, kLineNumberOffset, tab_title_x_coords,
                       actual_tab_title_widths);

    image_renderer.draw(size, tab->scroll, editor_offset, tab_title_x_coords,
                        actual_tab_title_widths);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderUiText(size, tab->end_caret, color_scheme, editor_offset, tabs,
                               tab_title_x_coords);

    // Cleanup.
    selection_renderer.destroyInstances();
}
}
