#include "fast_startup_app.h"

#include "base/filesystem/file_reader.h"
#include "gui/renderer/renderer.h"
#include "opengl/functions_gl.h"

#include "util/std_print.h"

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void FastStartupApp::onLaunch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    // Load fonts.
    auto& font_rasterizer = font::FontRasterizer::instance();
    main_font_id = font_rasterizer.addFont(kMainFontFace, kMainFontSize);
    ui_font_id = font_rasterizer.addSystemFont(kUIFontSize);

    createWindow();
}

void FastStartupApp::createWindow() {
    std::unique_ptr<FastStartupWindow> editor_window =
        std::make_unique<FastStartupWindow>(*this, 1200, 800, 0);

    editor_window->show();
    editor_windows.emplace_back(std::move(editor_window));
}

void FastStartupApp::destroyWindow(int wid) {
    editor_windows[wid] = nullptr;
}
