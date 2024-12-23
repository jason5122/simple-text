#include "resizable_widget_app.h"

#include "base/filesystem/file_reader.h"
#include "gui/renderer/renderer.h"
#include "opengl/functions_gl.h"

#include <fmt/base.h>

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void ResizableWidgetApp::onLaunch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    createWindow();
}

void ResizableWidgetApp::createWindow() {
    std::unique_ptr<ResizableWidgetWindow> editor_window =
        std::make_unique<ResizableWidgetWindow>(*this, 1200, 800, 0);

    editor_window->show();
    editor_windows.emplace_back(std::move(editor_window));
}

void ResizableWidgetApp::destroyWindow(int wid) {
    editor_windows[wid] = nullptr;
}
