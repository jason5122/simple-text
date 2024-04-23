#include "build/buildflag.h"
#include "simple_text.h"
#include <glad/glad.h>

using EditorWindow = SimpleText::EditorWindow;

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent, width, height), parent(parent), wid(wid) {}

#include <iostream>

EditorWindow::~EditorWindow() {
    std::cerr << "~EditorWindow " << wid << '\n';
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    GLfloat red = colors::background.r / 255.0;
    GLfloat green = colors::background.g / 255.0;
    GLfloat blue = colors::background.b / 255.0;
    glClearColor(red, green, blue, 1.0);

    // fs::path file_path = ResourcePath() / "sample_files/example.json";
    // fs::path file_path = ResourcePath() / "sample_files/worst_case.json";
    fs::path file_path = ResourcePath() / "sample_files/sort.scm";

    buffer.setContents(ReadFile(file_path));
    highlighter.setLanguage("source.json");

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);

#if IS_LINUX
    // TODO: Implement scale factor support.
    std::string main_font = "Source Code Pro";
    std::string ui_font = "Noto Sans";
    int main_font_size = 16 * 2;
    int ui_font_size = 11 * 2;

    main_font_rasterizer.setup(0, main_font, main_font_size);
    ui_font_rasterizer.setup(1, ui_font, ui_font_size);

    text_renderer.setup(main_font_rasterizer);
    rect_renderer.setup();
    image_renderer.setup();
#endif
}

void EditorWindow::onDraw() {
    renderer::Size size{
        .width = width() * scaleFactor(),
        .height = height() * scaleFactor(),
    };

    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    int status_bar_height = ui_font_rasterizer.line_height;

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderText(size, scroll, buffer, highlighter, editor_offset,
                             main_font_rasterizer, status_bar_height, start_cursor, end_cursor,
                             longest_line_x);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, scroll, end_cursor, main_font_rasterizer.line_height,
                       buffer.lineCount(), longest_line_x, editor_offset, status_bar_height);

    image_renderer.draw(size, scroll, editor_offset);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderUiText(size, main_font_rasterizer, ui_font_rasterizer, end_cursor);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createWindow();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        close();
    }

    if (key == app::Key::kA && modifiers == app::kPrimaryModifier) {
        parent.createNWindows(25);
    }
    if (key == app::Key::kB && modifiers == app::kPrimaryModifier) {
        parent.destroyAllWindows();
    }

    if (key == app::Key::kC && modifiers == app::kPrimaryModifier) {
        close();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
