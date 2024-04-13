#include "build/buildflag.h"
#include "editor_window.h"

EditorWindow::EditorWindow(SimpleText& parent, int width, int height)
    : Window(parent, width, height), parent(parent) {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

    // fs::path file_path = ResourcePath() / "sample_files/example.json";
    // fs::path file_path = ResourcePath() / "sample_files/worst_case.json";
    fs::path file_path = ResourcePath() / "sample_files/sort.scm";

    buffer.setContents(ReadFile(file_path));
    highlighter.setLanguage("source.json");

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);

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

    text_renderer.setup(width, height, main_font_rasterizer);
    rect_renderer.setup(width, height);
    image_renderer.setup(width, height);
}

void EditorWindow::onDraw() {
    int status_bar_height = ui_font_rasterizer.line_height;

    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderText(scroll_x, scroll_y, buffer, highlighter, editor_offset_x,
                             editor_offset_y, main_font_rasterizer, status_bar_height);

    // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    // rect_renderer.draw(scroll_x, scroll_y, text_renderer.cursor_end_x,
    //                    text_renderer.cursor_end_line, main_font_rasterizer.line_height,
    //                    buffer.lineCount(), text_renderer.longest_line_x, editor_offset_x,
    //                    editor_offset_y, status_bar_height);

    // image_renderer.draw(scroll_x, scroll_y, editor_offset_x, editor_offset_y);

    // glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    // text_renderer.renderUiText(main_font_rasterizer, ui_font_rasterizer);
}

void EditorWindow::onResize(int width, int height) {
    glViewport(0, 0, width, height);
    // text_renderer.resize(width, height);
    // rect_renderer.resize(width, height);
    // image_renderer.resize(width, height);

    redraw();
}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createChild();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.destroyChild(this);
    }
}
