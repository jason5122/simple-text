#include "editor_window.h"
#include "simple_text/simple_text.h"

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), wid(wid), parent(parent), color_scheme(isDarkMode()) {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    // TODO: Move setup to SimpleText. We currently need this here because of GTK.
    parent.renderer.setup();
}

void EditorWindow::onDraw() {
    parent.renderer.draw();
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
