#include "build/buildflag.h"
#include "editor_window.h"
#include "renderer/opengl_functions.h"
#include "simple_text/simple_text.h"
#include <cctype>

#include <format>
#include <iostream>

// TODO: Temporary; remove this.
#include "gui/functions_gl.h"

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode()) {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(0.5f, 0.0f, 0.0f, 1.0f);

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    std::cerr << std::format("tex_id = {}", tex_id) << '\n';
}

void EditorWindow::onDraw() {
    gui::FunctionsGL functions;
    functions.initialize();
    // glClear(GL_COLOR_BUFFER_BIT);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
