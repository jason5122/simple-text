#include "build/buildflag.h"
#include "editor_window.h"
#include "simple_text/simple_text.h"
#include <cctype>

#include <glad/glad.h>
#include <iostream>

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid) {}

EditorWindow::~EditorWindow() {
    std::cerr << "~EditorWindow " << wid << '\n';
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
