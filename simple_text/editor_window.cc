#include "editor_window.h"
#include "simple_text/simple_text.h"

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), wid(wid), parent(parent), color_scheme(isDarkMode()) {}

void EditorWindow::onOpenGLActivate(int width, int height) {}

void EditorWindow::onDraw(int width, int height) {
    // renderer::Size size{
    //     .width = width() * scaleFactor(),
    //     .height = height() * scaleFactor(),
    // };
    renderer::Size size{width, height};

    std::cerr << std::format("width = {}, height = {}", size.width, size.height) << '\n';
    parent.renderer.draw(size);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
