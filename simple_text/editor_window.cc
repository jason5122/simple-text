#include "build/buildflag.h"
#include "editor_window.h"
#include "renderer/opengl_functions.h"
#include "simple_text/simple_text.h"
#include <cctype>

#include <format>
#include <iostream>

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode()) {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    std::cerr << std::format("tex_id = {}", tex_id) << '\n';
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
