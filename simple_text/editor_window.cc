#include "editor_window.h"
#include "simple_text/editor_app.h"

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window(parent), wid(wid), parent(parent), color_scheme(isDarkMode()) {
    buffer.setContents("Hello world!\nLorem ipsum");
}

void EditorWindow::onOpenGLActivate(int width, int height) {}

void EditorWindow::onDraw(int width, int height) {
    parent.renderer.draw({width, height}, buffer);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
