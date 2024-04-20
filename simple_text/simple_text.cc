#include "simple_text.h"
#include "simple_text/editor_window.h"

void SimpleText::onLaunch() {
    // TODO: Implement scale factor support.
    std::string main_font = "Source Code Pro";
#if IS_MAC
    std::string ui_font = "SF Pro Text";
    int main_font_size = 16 * 2;
    int ui_font_size = 11 * 2;
#elif IS_LINUX
    std::string ui_font = "Noto Sans";
    int main_font_size = 16 * 2;
    int ui_font_size = 11 * 2;
#elif IS_WIN
    std::string ui_font = "Segoe UI";
    int main_font_size = 12 * 2;
    int ui_font_size = 9 * 2;
#endif
    main_font_rasterizer.setup(0, main_font, main_font_size);
    ui_font_rasterizer.setup(1, ui_font, ui_font_size);

    createWindow();
}

void SimpleText::createWindow() {
    EditorWindow* editor_window = new EditorWindow(*this, 600, 400);
    editor_window->show();
}

void SimpleText::destroyWindow(EditorWindow* editor_window) {
    // FIXME: Make this work properly for GTK.
    // delete editor_window;
}

#include <iostream>

void SimpleText::createAllWindows() {
    for (int i = 0; i < 20; i++) {
        EditorWindow* editor_window = new EditorWindow(*this, 600, 400);
        editor_window->show();
        editor_windows.push_back(editor_window);

        std::cerr << editor_windows.size() << '\n';
    }
}

void SimpleText::destroyAllWindows() {
    for (const auto& editor_window : editor_windows) {
        editor_window->close();
    }
    editor_windows.clear();
}
