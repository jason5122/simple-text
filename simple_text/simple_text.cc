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

    createChild();
}

void SimpleText::createChild() {
    EditorWindow* editor_window = new EditorWindow(*this, 600, 400);
    editor_window->show();

    // TODO: Debug; remove this.
    incrementWindowCount();
}

void SimpleText::destroyChild(EditorWindow* editor_window) {
    editor_window->close();
    delete editor_window;
}
