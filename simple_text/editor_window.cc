#include "build/buildflag.h"
#include "gui/key.h"
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

    Rgb& background = parent.color_scheme.background;
    GLfloat red = background.r / 255.0;
    GLfloat green = background.g / 255.0;
    GLfloat blue = background.b / 255.0;
    glClearColor(red, green, blue, 1.0);

    // fs::path file_path = ResourceDir() / "sample_files/example.json";
    // fs::path file_path = ResourceDir() / "sample_files/worst_case.json";
    fs::path file_path = ResourceDir() / "sample_files/sort.scm";
    // fs::path file_path = ResourceDir() / "sample_files/proportional_font_test.json";
    fs::path file_path2 = ResourceDir() / "sample_files/proportional_font_test.json";

    buffer.setContents(ReadFile(file_path));
    highlighter.setLanguage("source.json", parent.color_scheme);
    buffer2.setContents(ReadFile(file_path2));
    highlighter2.setLanguage("source.json", parent.color_scheme);

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);
    TSInput input2 = {&buffer2, Buffer::read, TSInputEncodingUTF8};
    highlighter2.parse(input2);

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
    selection_renderer.setup(main_font_rasterizer);
#endif
}

void EditorWindow::onDraw() {
    using Selection = renderer::SelectionRenderer::Selection;

    renderer::Size size{
        .width = width() * scaleFactor(),
        .height = height() * scaleFactor(),
    };

    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
    FontRasterizer& ui_font_rasterizer = parent.ui_font_rasterizer;
    renderer::TextRenderer& text_renderer = parent.text_renderer;
    renderer::RectRenderer& rect_renderer = parent.rect_renderer;
    renderer::ImageRenderer& image_renderer = parent.image_renderer;
    renderer::SelectionRenderer& selection_renderer = parent.selection_renderer;
#endif

    int status_bar_height = ui_font_rasterizer.line_height;

    Buffer& selected_buffer = use_buffer2 ? buffer2 : buffer;
    SyntaxHighlighter& selected_highlighter = use_buffer2 ? highlighter2 : highlighter;

    std::vector<Selection> selections =
        text_renderer.getSelections(selected_buffer, main_font_rasterizer, start_caret, end_caret);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderText(size, scroll, selected_buffer, selected_highlighter, editor_offset,
                             main_font_rasterizer, status_bar_height, start_caret, end_caret,
                             longest_line_x, parent.color_scheme);
    selection_renderer.render(size, scroll, editor_offset, main_font_rasterizer, selections);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, scroll, end_caret, main_font_rasterizer.line_height,
                       selected_buffer.lineCount(), longest_line_x, editor_offset,
                       status_bar_height, parent.color_scheme);

    image_renderer.draw(size, scroll, editor_offset);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderUiText(size, main_font_rasterizer, ui_font_rasterizer, end_caret,
                               parent.color_scheme);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(float dx, float dy) {
    // TODO: Uncomment this while not testing.
    // scroll.x += dx;
    scroll.y += dy;

    redraw();
}

void EditorWindow::onLeftMouseDown(float mouse_x, float mouse_y) {
    renderer::Point mouse{
        .x = mouse_x - editor_offset.x + scroll.x,
        .y = mouse_y - editor_offset.y + scroll.y,
    };

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
    renderer::TextRenderer& text_renderer = parent.text_renderer;
#endif

    text_renderer.setCaretInfo(buffer, main_font_rasterizer, mouse, start_caret);
    end_caret = start_caret;

    redraw();
}

void EditorWindow::onLeftMouseDrag(float mouse_x, float mouse_y) {
    renderer::Point mouse{
        .x = mouse_x - editor_offset.x + scroll.x,
        .y = mouse_y - editor_offset.y + scroll.y,
    };

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
    renderer::TextRenderer& text_renderer = parent.text_renderer;
#endif

    text_renderer.setCaretInfo(buffer, main_font_rasterizer, mouse, end_caret);

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

    if (key == app::Key::kJ && modifiers == app::kPrimaryModifier) {
        use_buffer2 = !use_buffer2;
        redraw();
    }
    if (key == app::Key::kK && modifiers == app::kPrimaryModifier) {
        use_buffer2 = !use_buffer2;
        redraw();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
