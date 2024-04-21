#include "build/buildflag.h"
#include "editor_window.h"
#include <glad/glad.h>

EditorWindow::EditorWindow(SimpleText& parent, int width, int height)
    : Window(parent, width, height), parent(parent), memory_waster(1000000, 1) {}

#include <iostream>

EditorWindow::~EditorWindow() {
    std::cerr << "~EditorWindow\n";
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(0.0, 1.0, 0.0, 1.0);
}

void EditorWindow::onDraw() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void EditorWindow::onResize(int width, int height) {
    glViewport(0, 0, width, height);
    redraw();
}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createWindow();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        close();
    }

    if (key == app::Key::kA && modifiers == app::kPrimaryModifier) {
        parent.createAllWindows();
    }
    if (key == app::Key::kB && modifiers == app::kPrimaryModifier) {
        parent.destroyAllWindows();
    }
}

#include <iostream>

void EditorWindow::onClose() {
    std::cerr << "onClose\n";
    parent.destroyWindow(this);
}
