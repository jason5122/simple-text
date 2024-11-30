#include "editor_app.h"

#include "base/filesystem/file_reader.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/renderer.h"
#include "opengl/functions_gl.h"

// TODO: Debug; remove this.
#include "util/std_print.h"

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::onLaunch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    // Load fonts.
    auto& font_rasterizer = font::FontRasterizer::instance();
    main_font_id = font_rasterizer.addFont(kMainFontFace, kMainFontSize);
    ui_font_id = font_rasterizer.addSystemFont(kUIFontSize);

    // Load images.
    auto& image_renderer = gui::Renderer::instance().getImageRenderer();
    std::string panel_close_2x = std::format("{}/icons/panel_close@2x.png", base::ResourceDir());
    std::string folder_open_2x = std::format("{}/icons/folder_open@2x.png", base::ResourceDir());
    panel_close_image_id = image_renderer.addPng(panel_close_2x);
    folder_open_image_id = image_renderer.addPng(folder_open_2x);

    createWindow();
}

void EditorApp::onQuit() {
    std::println("SimpleText::onQuit()");
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
    editor_windows.emplace_back(std::move(editor_window));
}

void EditorApp::destroyWindow(int wid) {
    std::println("EditorApp::destroyWindow({})", wid);
    editor_windows[wid] = nullptr;
}
