#include "build/buildflag.h"
#include "simple_text.h"
#include <memory>

SimpleText::SimpleText()
    : file_watcher(DataDir(), this)
#if IS_MAC || IS_WIN
      ,
      renderer(main_font_rasterizer, ui_font_rasterizer)
#endif
{
}

SimpleText::~SimpleText() {}

void SimpleText::onLaunch() {
    // TODO: Implement scale factor support.
    // std::string main_font = "Arial";
    std::string main_font = "Source Code Pro";
#if IS_MAC
    main_font = "Menlo";
    std::string ui_font = "SF Pro Text";
    int main_font_size = 16 * 2;
    int ui_font_size = 11 * 2;
#elif IS_WIN
    // main_font = "Consolas";
    std::string ui_font = "Segoe UI";
    int main_font_size = 11 * 2;
    int ui_font_size = 9 * 2;
#elif IS_LINUX
    main_font = "Monospace";
    std::string ui_font = "Noto Sans";
    int main_font_size = 12 * 2;
    int ui_font_size = 9 * 2;
#endif
    main_font_rasterizer.setup(0, main_font, main_font_size);
    ui_font_rasterizer.setup(1, ui_font, ui_font_size);

#if IS_MAC || IS_WIN
    renderer.setup();
#endif

    createWindow();

    file_watcher.start();
}

void SimpleText::onFileEvent() {
    // TODO: Investigate if redrawing *all* windows is ever a performance problem.
    for (const auto& editor_window : editor_windows) {
        editor_window->reloadColorScheme();
    }
    key_bindings.reload();
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

void SimpleText::createNWindows(int n) {
    for (int i = 0; i < n; i++) {
        createWindow();
    }
}

void SimpleText::destroyAllWindows() {
    // FIXME: Why does this crash on Windows when iterating over *all* windows?
    for (size_t i = editor_windows.size() - 1; i >= 1; i--) {
        if (editor_windows[i]) {
            editor_windows[i]->close();
        }
    }
}
