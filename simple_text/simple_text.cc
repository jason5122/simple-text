#include "simple_text.h"
#include <iostream>
#include <memory>

SimpleText::SimpleText() : gl(std::make_unique<opengl::FunctionsGL>()), renderer(gl.get()) {}

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void SimpleText::onLaunch() {
    gl->initialize();
    renderer.setup();

    createWindow();
    createWindow();
}

void SimpleText::onQuit() {
    std::cerr << "SimpleText::onQuit()\n";
}

void SimpleText::createWindow() {
    std::unique_ptr<EditorWindow> editor_window =
        std::make_unique<EditorWindow>(*this, 600, 400, editor_windows.size());
    editor_window->show();
    editor_windows.push_back(std::move(editor_window));
}

void SimpleText::destroyWindow(int wid) {
    editor_windows[wid] = nullptr;
}
