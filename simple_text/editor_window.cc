#include "build/buildflag.h"
#include "editor_window.h"
#include "renderer/opengl_functions.h"
#include "simple_text/simple_text.h"
#include <cctype>
#include <iostream>

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode()) {}

EditorWindow::~EditorWindow() {
    std::cerr << "~EditorWindow " << wid << '\n';
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
}

void EditorWindow::onDraw() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
