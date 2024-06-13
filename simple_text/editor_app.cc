#include "editor_app.h"
#include <memory>

#include <format>
#include <iostream>

EditorApp::EditorApp() : gl{std::make_unique<opengl::FunctionsGL>()}, renderer{gl.get()} {}

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::onLaunch() {
    gl->initialize();
    renderer.setup();

    createWindow();
    createWindow();
}

void EditorApp::onQuit() {
    std::cerr << "SimpleText::onQuit()\n";
}

void EditorApp::createWindow() {
    std::unique_ptr<EditorWindow> editor_window =
        std::make_unique<EditorWindow>(*this, 600, 400, editor_windows.size());
    editor_window->show();
    editor_windows.push_back(std::move(editor_window));
}

void EditorApp::destroyWindow(int wid) {
    std::cerr << std::format("SimpleText: destroy window {}", wid) << '\n';

    editor_windows[wid] = nullptr;
}
