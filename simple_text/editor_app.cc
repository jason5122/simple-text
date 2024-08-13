#include "editor_app.h"
#include "opengl/functions_gl.h"

// TODO: Debug; remove this.
#include <format>
#include <iostream>

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::onLaunch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    createWindow();
}

void EditorApp::onQuit() {
    std::cerr << "SimpleText::onQuit()\n";
}

void EditorApp::onAppAction(app::AppAction action) {
    if (action == app::AppAction::kNewFile) {
        createWindow();
    }
    if (action == app::AppAction::kNewWindow) {
        createWindow();
    }
}

void EditorApp::createWindow() {
    std::unique_ptr<EditorWindow> editor_window =
        std::make_unique<EditorWindow>(*this, 1200, 800, editor_windows.size());

#ifdef NDEBUG
    const std::string& debug_or_release = "Release";
#else
    const std::string& debug_or_release = "Debug";
#endif
    editor_window->setTitle(std::format("Simple Text ({})", debug_or_release));

    editor_window->show();
    editor_windows.push_back(std::move(editor_window));
}

void EditorApp::destroyWindow(int wid) {
    std::cerr << std::format("SimpleText: destroy window {}", wid) << '\n';
    editor_windows[wid] = nullptr;
}
