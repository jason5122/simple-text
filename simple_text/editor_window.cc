#include "build/buildflag.h"
#include "editor_window.h"
#include "opengl/functionsgl_enums.h"
#include "simple_text/simple_text.h"
#include <cctype>

#include <format>
#include <iostream>

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode()) {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    parent.gl->enable(GL_BLEND);
    parent.gl->depthMask(GL_FALSE);

    parent.gl->clearColor(0.5f, 0.0f, 0.0f, 1.0f);

    GLuint tex_id;
    parent.gl->genTextures(1, &tex_id);
    std::cerr << std::format("tex_id = {}", tex_id) << '\n';
}

void EditorWindow::onDraw() {
    GLenum err;
    while ((err = parent.gl->getError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error\n";
    }

    parent.gl->clear(GL_COLOR_BUFFER_BIT);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
