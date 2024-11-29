#include "editor_app.h"

#include "opengl/functions_gl.h"

#include "util/std_print.h"

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::onLaunch() {
    std::println("alignof(EditorApp) = {}", alignof(EditorApp));
    std::println("alignof(EditorWindow) = {}", alignof(EditorWindow));
    std::println("std::alignment_of_v<EditorApp> = {}", std::alignment_of_v<EditorApp>);

    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    createWindow();
}

void EditorApp::createWindow() {
    std::unique_ptr<EditorWindow> editor_window =
        std::make_unique<EditorWindow>(*this, 1200, 800, 0);

    editor_window->show();
    win = std::move(editor_window);
}

void EditorApp::destroyWindow(int wid) {
    win = nullptr;
}
