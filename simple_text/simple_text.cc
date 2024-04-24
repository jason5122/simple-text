#include "build/buildflag.h"
#include "simple_text.h"
#include <memory>

SimpleText::SimpleText() {}

SimpleText::~SimpleText() {}

void SimpleText::onLaunch() {
#if IS_MAC || IS_WIN

    // TODO: Implement scale factor support.
    // std::string main_font = "Arial";
    std::string main_font = "Source Code Pro";
#if IS_MAC
    std::string ui_font = "SF Pro Text";
    int main_font_size = 16 * 2;
    int ui_font_size = 11 * 2;
#elif IS_WIN
    std::string ui_font = "Segoe UI";
    int main_font_size = 12 * 2;
    int ui_font_size = 9 * 2;
#endif

    main_font_rasterizer.setup(0, main_font, main_font_size);
    ui_font_rasterizer.setup(1, ui_font, ui_font_size);

    text_renderer.setup(main_font_rasterizer);
    rect_renderer.setup();
    image_renderer.setup();
#endif

    createWindow();
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
