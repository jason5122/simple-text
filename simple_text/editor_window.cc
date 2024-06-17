#include "editor_window.h"
#include "simple_text/editor_app.h"

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window(parent), wid(wid), parent(parent), color_scheme(isDarkMode()) {
    buffer.setContents(R"(#include <memory>

class Foo {
public:
    Foo(std::shared_ptr<int> p);
private:
    std::shared_ptr<int> ptr;
};

Foo::Foo(std::shared_ptr<int> p) : ptr(std::move(p)) {
}
)");
}

void EditorWindow::onOpenGLActivate(int width, int height) {}

void EditorWindow::onDraw(int width, int height) {
    parent.renderer->draw({width, height}, buffer, scroll_offset, end_caret);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    // scroll_offset.x += dx;
    scroll_offset.y += dy;
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x, int mouse_y, app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    int line_number_offset = 100;
    renderer::Point mouse{
        .x = mouse_x + scroll_offset.x - 200 * 2 - line_number_offset,
        .y = mouse_y + scroll_offset.y - 30 * 2,
    };

    parent.renderer->movement.setCaretInfo(buffer, mouse, end_caret);

    redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    int line_number_offset = 100;
    renderer::Point mouse{
        .x = mouse_x + scroll_offset.x - 200 * 2 - line_number_offset,
        .y = mouse_y + scroll_offset.y - 30 * 2,
    };

    parent.renderer->movement.setCaretInfo(buffer, mouse, end_caret);

    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
