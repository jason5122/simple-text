#include "base/path_service.h"
#include "font/font_rasterizer.h"
#include "gl/loader.h"
#include "gui/renderer/renderer.h"
#include "simple_text/editor_app.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace gui {

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::on_launch() {
    gl::load_global_function_pointers();

    // Load fonts.
    auto& font_rasterizer = font::FontRasterizer::instance();
    main_font_id = font_rasterizer.add_font(kMainFontFace, kMainFontSize);
    ui_font_small_id = font_rasterizer.add_system_font(kUIFontSizeSmall);
    ui_font_regular_id = font_rasterizer.add_system_font(kUIFontSizeRegular);

    // Load images.
    base::FilePath assets_path;
    base::PathService::get(base::PathKey::kDirAssets, &assets_path);
    base::FilePath icons_path = assets_path.Append(FILE_PATH_LITERAL("icons"));

    auto panel_close_2x = icons_path.Append(FILE_PATH_LITERAL("panel_close@2x.png"));
    auto folder_open_2x = icons_path.Append(FILE_PATH_LITERAL("folder_open@2x.png"));
    auto icon_regex_2x = icons_path.Append(FILE_PATH_LITERAL("icon_regex@2x.png"));
    auto icon_case_sensitive_2x =
        icons_path.Append(FILE_PATH_LITERAL("icon_case_sensitive@2x.png"));
    auto icon_whole_word_2x = icons_path.Append(FILE_PATH_LITERAL("icon_whole_word@2x.png"));
    auto icon_wrap_2x = icons_path.Append(FILE_PATH_LITERAL("icon_wrap@2x.png"));
    auto icon_in_selection_2x = icons_path.Append(FILE_PATH_LITERAL("icon_in_selection@2x.png"));
    auto icon_highlight_matches_2x =
        icons_path.Append(FILE_PATH_LITERAL("icon_highlight_matches@2x.png"));

    auto& texture_cache = gui::Renderer::instance().texture_cache();
    panel_close_image_id = texture_cache.add_png(panel_close_2x);
    folder_open_image_id = texture_cache.add_png(folder_open_2x);
    icon_regex_image_id = texture_cache.add_png(icon_regex_2x);
    icon_case_sensitive_image_id = texture_cache.add_png(icon_case_sensitive_2x);
    icon_whole_word_image_id = texture_cache.add_png(icon_whole_word_2x);
    icon_wrap_image_id = texture_cache.add_png(icon_wrap_2x);
    icon_in_selection_id = texture_cache.add_png(icon_in_selection_2x);
    icon_highlight_matches_id = texture_cache.add_png(icon_highlight_matches_2x);

    create_window();
}

void EditorApp::on_quit() { spdlog::info("SimpleText::onQuit()"); }

void EditorApp::on_app_action(AppAction action) {
    if (action == AppAction::kNewFile) {
        create_window();
    }
    if (action == AppAction::kNewWindow) {
        create_window();
    }
}

void EditorApp::create_window() {
    auto editor_window = std::make_unique<EditorWindow>(*this, 1200, 800, editor_windows.size());

#ifdef NDEBUG
    const std::string& debug_or_release = "Release";
#else
    const std::string& debug_or_release = "Debug";
#endif
    editor_window->set_title(fmt::format("Simple Text ({})", debug_or_release));

    editor_window->show();
    editor_windows.emplace_back(std::move(editor_window));
}

void EditorApp::destroy_window(int wid) {
    spdlog::info("EditorApp::destroyWindow({})", wid);
    editor_windows[wid] = nullptr;
}

}  // namespace gui
