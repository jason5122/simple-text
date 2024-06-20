#include "editor_window.h"
#include "gui/side_bar_widget.h"
#include "gui/tab_bar_widget.h"
#include "gui/text_view_widget.h"
#include "simple_text/editor_app.h"
#include "util/profile_util.h"

namespace {
const std::string sample_text = R"(#include "opengl/functions_gl_enums.h"
#include "renderer.h"

namespace renderer {

Renderer::Renderer(std::shared_ptr<opengl::FunctionsGL> shared_gl)
    : gl{std::move(shared_gl)},
      main_glyph_cache{gl, "Source Code Pro", 16 * 2},
      ui_glyph_cache{gl, "Arial", 11 * 2},
      text_renderer{gl, main_glyph_cache, ui_glyph_cache},
      rect_renderer{gl},
      movement{main_glyph_cache} {
    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);
}

void Renderer::draw(const Size& size,
                    const base::Buffer& buffer,
                    const Point& scroll_offset,
                    const CaretInfo& end_caret) {
    gl->viewport(0, 0, size.width, size.height);

    gl->clear(GL_COLOR_BUFFER_BIT);

    int longest_line;
    Point end_caret_pos;

    gl->blendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderText(size, scroll_offset, buffer, editor_offset, end_caret, end_caret,
                             longest_line, end_caret_pos);

    gl->blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, scroll_offset, end_caret_pos, main_glyph_cache.lineHeight(), 50, 1000,
                       editor_offset, ui_glyph_cache.lineHeight());
}

}

#include "opengl/functions_gl_enums.h"
#include "renderer.h"

namespace renderer {

Renderer::Renderer(std::shared_ptr<opengl::FunctionsGL> shared_gl)
    : gl{std::move(shared_gl)},
      main_glyph_cache{gl, "Source Code Pro", 16 * 2},
      ui_glyph_cache{gl, "Arial", 11 * 2},
      text_renderer{gl, main_glyph_cache, ui_glyph_cache},
      rect_renderer{gl},
      movement{main_glyph_cache} {
    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);
}
)";
}

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window{parent}, wid{wid}, parent{parent}, main_widget{parent.renderer} {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    int tab_bar_height = 30 * 2;
    int side_bar_width = 200 * 2;
    main_widget.addChild(std::make_unique<gui::SideBarWidget>(parent.renderer, side_bar_width));
    // main_widget.addChild(std::make_unique<gui::TabBarWidget>(parent.renderer, tab_bar_height));

    std::unique_ptr<gui::TextViewWidget> text_view_widget =
        std::make_unique<gui::TextViewWidget>(parent.renderer);
    text_view_widget->setContents(sample_text);
    main_widget.addChild(std::move(text_view_widget));
}

void EditorWindow::onDraw(int width, int height) {
    {
        PROFILE_BLOCK("render");
        // TODO: Move Renderer::flush() to GUI toolkit instead of calling this directly.
        main_widget.draw({width, height}, {0, 0});
        parent.renderer->flush({width, height});
    }
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    main_widget.scroll({dx, dy});
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    // int line_number_offset = 100;
    // renderer::Point mouse{
    //     .x = mouse_x + scroll_offset.x - 200 * 2 - line_number_offset,
    //     .y = mouse_y + scroll_offset.y - 30 * 2,
    // };

    // parent.renderer->getMovement().setCaretInfo(buffer, mouse, end_caret);

    // redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    // int line_number_offset = 100;
    // renderer::Point mouse{
    //     .x = mouse_x + scroll_offset.x - 200 * 2 - line_number_offset,
    //     .y = mouse_y + scroll_offset.y - 30 * 2,
    // };

    // parent.renderer->getMovement().setCaretInfo(buffer, mouse, end_caret);

    // redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
