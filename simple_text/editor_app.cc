#include "build/build_config.h"
#include "editor_app.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/renderer.h"
#include "opengl/functions_gl.h"

// TODO: Debug; remove this.
#include "util/std_print.h"

// TODO: Properly load this from settings.
namespace {

#if BUILDFLAG(IS_MAC)
constexpr int kMainFontSize = 16 * 2;
constexpr int kUIFontSize = 11 * 2;
// const std::string kMainFontFace = "Monaco";
// const std::string kMainFontFace = "Fira Code";
const std::string kMainFontFace = "Source Code Pro";
// const std::string kMainFontFace = "SF Pro Text";
// const std::string kMainFontFace = "Courier";
// const std::string kMainFontFace = "Courier New";
// const std::string kMainFontFace = "Andale Mono";
// const std::string kMainFontFace = "Arial";
// const std::string kMainFontFace = "Menlo";
// const std::string kMainFontFace = "Charter";
// const std::string kMainFontFace = "Times New Roman";
// const std::string kUIFontFace = "SF Pro Text";
const std::string kUIFontFace = "SF Pro Text";
#elif BUILDFLAG(IS_WIN)
constexpr int kMainFontSize = 11 * 2;
constexpr int kUIFontSize = 8 * 2;
const std::string kMainFontFace = "Source Code Pro";
// const std::string kMainFontFace = "Consolas";
// const std::string kMainFontFace = "Cascadia Code";
const std::string kUIFontFace = "Segoe UI";
#elif BUILDFLAG(IS_LINUX)
constexpr int kMainFontSize = 12 * 2;
constexpr int kUIFontSize = 11 * 2;
const std::string kMainFontFace = "Monospace";
const std::string kUIFontFace = "Arial";
#endif

// const std::string kMainFontFace = "Source Code Pro";
// const std::string kMainFontFace = "Fira Code";

}  // namespace

// We should have an OpenGL context within this function.
// Load OpenGL function pointers and perform OpenGL setup here.
void EditorApp::onLaunch() {
    opengl::FunctionsGL functions_gl{};
    functions_gl.loadGlobalFunctionPointers();

    auto& font_rasterizer = font::FontRasterizer::instance();
    size_t main_font_id = font_rasterizer.addFont(kMainFontFace, kMainFontSize);
    size_t ui_font_id = font_rasterizer.addFont(kUIFontFace, kUIFontSize);

    auto& glyph_cache = gui::Renderer::instance().getGlyphCache();
    glyph_cache.setMainFontId(main_font_id);
    glyph_cache.setUIFontId(ui_font_id);

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
    editor_windows.push_back(std::move(editor_window));
}

void EditorApp::destroyWindow(int wid) {
    std::println("SimpleText: destroy window {}", wid);
    editor_windows[wid] = nullptr;
}
