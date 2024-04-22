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

    glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

    // fs::path file_path = ResourcePath() / "sample_files/example.json";
    // fs::path file_path = ResourcePath() / "sample_files/worst_case.json";
    fs::path file_path = ResourcePath() / "sample_files/sort.scm";

    buffer.setContents(ReadFile(file_path));
    highlighter.setLanguage("source.json");

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);

    text_renderer.setup(width, height, parent.main_font_rasterizer);
    rect_renderer.setup(width, height);
}

void EditorWindow::onDraw() {
    int scaled_width = width() * scaleFactor();
    int scaled_height = height() * scaleFactor();

    glViewport(0, 0, scaled_width, scaled_height);
    glClear(GL_COLOR_BUFFER_BIT);

    int status_bar_height = parent.ui_font_rasterizer.line_height;

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderText(scroll_x, scroll_y, buffer, highlighter, editor_offset_x,
                             editor_offset_y, parent.main_font_rasterizer, status_bar_height);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(scroll_x, scroll_y, text_renderer.cursor_end_x,
                       text_renderer.cursor_end_line, parent.main_font_rasterizer.line_height,
                       buffer.lineCount(), text_renderer.longest_line_x, editor_offset_x,
                       editor_offset_y, status_bar_height);

    parent.image_renderer.draw(scaled_width, scaled_height, scroll_x, scroll_y, editor_offset_x,
                               editor_offset_y);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderUiText(parent.main_font_rasterizer, parent.ui_font_rasterizer);
}

void EditorWindow::onResize(int width, int height) {
    glViewport(0, 0, width, height);
    text_renderer.resize(width, height);
    rect_renderer.resize(width, height);

    redraw();
}

void EditorWindow::onScroll(float dx, float dy) {
    // TODO: Uncomment this while not testing.
    // scroll_x += dx;
    scroll_y += dy;

    redraw();
}

void EditorWindow::onLeftMouseDown(float mouse_x, float mouse_y) {
    mouse_x -= editor_offset_x;
    mouse_y -= editor_offset_y;
    mouse_x += scroll_x;
    mouse_y += scroll_y;

    cursor_start_x = mouse_x;
    cursor_start_y = mouse_y;

    text_renderer.setCursorPositions(buffer, cursor_start_x, cursor_start_y, mouse_x, mouse_y,
                                     parent.main_font_rasterizer);

    redraw();
}

void EditorWindow::onLeftMouseDrag(float mouse_x, float mouse_y) {
    mouse_x -= editor_offset_x;
    mouse_y -= editor_offset_y;
    mouse_x += scroll_x;
    mouse_y += scroll_y;

    text_renderer.setCursorPositions(buffer, cursor_start_x, cursor_start_y, mouse_x, mouse_y,
                                     parent.main_font_rasterizer);

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
