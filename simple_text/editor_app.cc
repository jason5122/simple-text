#include "editor_app.h"

#include "base/files/file_reader.h"
#include "base/path_service.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/renderer.h"
#include "opengl/functions_gl.h"

// TODO: Debug; remove this.
#include <fmt/base.h>
#include <fmt/format.h>

namespace gui {

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::onLaunch() {
    opengl::FunctionsGL functions_gl;
    functions_gl.load_global_function_pointers();

    // Load fonts.
    auto& font_rasterizer = font::FontRasterizer::instance();
    main_font_id = font_rasterizer.add_font(kMainFontFace, kMainFontSize);
    ui_font_small_id = font_rasterizer.add_system_font(kUIFontSizeSmall);
    ui_font_regular_id = font_rasterizer.add_system_font(kUIFontSizeRegular);

    // Load images.
    base::FilePath assets_path;
    base::PathService::get(base::PathKey::kDirAssets, &assets_path);
    base::FilePath icons_path = assets_path.Append("icons");

    auto panel_close_2x = icons_path.Append("panel_close@2x.png");
    auto folder_open_2x = icons_path.Append("folder_open@2x.png");
    auto icon_regex_2x = icons_path.Append("icon_regex@2x.png");
    auto icon_case_sensitive_2x = icons_path.Append("icon_case_sensitive@2x.png");
    auto icon_whole_word_2x = icons_path.Append("icon_whole_word@2x.png");
    auto icon_wrap_2x = icons_path.Append("icon_wrap@2x.png");
    auto icon_in_selection_2x = icons_path.Append("icon_in_selection@2x.png");
    auto icon_highlight_matches_2x = icons_path.Append("icon_highlight_matches@2x.png");

    auto& texture_cache = gui::Renderer::instance().getTextureCache();
    panel_close_image_id = texture_cache.addPng(panel_close_2x.value());
    folder_open_image_id = texture_cache.addPng(folder_open_2x.value());
    icon_regex_image_id = texture_cache.addPng(icon_regex_2x.value());
    icon_case_sensitive_image_id = texture_cache.addPng(icon_case_sensitive_2x.value());
    icon_whole_word_image_id = texture_cache.addPng(icon_whole_word_2x.value());
    icon_wrap_image_id = texture_cache.addPng(icon_wrap_2x.value());
    icon_in_selection_id = texture_cache.addPng(icon_in_selection_2x.value());
    icon_highlight_matches_id = texture_cache.addPng(icon_highlight_matches_2x.value());

    createWindow();
}

void EditorApp::onQuit() {
    fmt::println("SimpleText::onQuit()");
}

void EditorApp::onAppAction(AppAction action) {
    if (action == AppAction::kNewFile) {
        createWindow();
    }
    if (action == AppAction::kNewWindow) {
        createWindow();
    }
}

void EditorApp::createWindow() {
    auto editor_window = std::make_unique<EditorWindow>(*this, 1200, 800, editor_windows.size());

#ifdef NDEBUG
    const std::string& debug_or_release = "Release";
#else
    const std::string& debug_or_release = "Debug";
#endif
    editor_window->setTitle(fmt::format("Simple Text ({})", debug_or_release));

    editor_window->show();
    editor_windows.emplace_back(std::move(editor_window));
}

void EditorApp::destroyWindow(int wid) {
    fmt::println("EditorApp::destroyWindow({})", wid);
    editor_windows[wid] = nullptr;
}

}  // namespace gui
