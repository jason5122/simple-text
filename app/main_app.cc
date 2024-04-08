#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "build/buildflag.h"
#include "font/rasterizer.h"
#include "ui/app/app.h"
#include "ui/app/app_window.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"
#include "util/profile_util.h"
#include <glad/glad.h>
#include <iostream>

class EditorWindow : public AppWindow {
public:
    EditorWindow(int id) : id(id) {}

    void onOpenGLActivate(int width, int height) {
        std::cerr << "id " << id << ": " << glGetString(GL_VERSION) << '\n';

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

    void onDraw() {
        {
            int status_bar_height = ui_font_rasterizer.line_height;

            PROFILE_BLOCK("id " + std::to_string(id) + ": redraw");
            glClear(GL_COLOR_BUFFER_BIT);

            glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
            text_renderer.renderText(scroll_x, scroll_y, buffer, highlighter, editor_offset_x,
                                     editor_offset_y, main_font_rasterizer, status_bar_height);

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
            rect_renderer.draw(scroll_x, scroll_y, text_renderer.cursor_end_x,
                               text_renderer.cursor_end_line, main_font_rasterizer.line_height,
                               buffer.lineCount(), text_renderer.longest_line_x, editor_offset_x,
                               editor_offset_y, status_bar_height);

            image_renderer.draw(scroll_x, scroll_y, editor_offset_x, editor_offset_y);

            glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
            text_renderer.renderUiText(main_font_rasterizer, ui_font_rasterizer);
        }
    }

    void onResize(int width, int height) {
        glViewport(0, 0, width, height);
        text_renderer.resize(width, height);
        rect_renderer.resize(width, height);
        image_renderer.resize(width, height);
    }

    void onScroll(float dx, float dy) {
        // TODO: Uncomment this while not testing.
        // scroll_x += dx;
        scroll_y += dy;
    }

    void onLeftMouseDown(float mouse_x, float mouse_y) {
        mouse_x -= editor_offset_x;
        mouse_y -= editor_offset_y;
        mouse_x += scroll_x;
        mouse_y += scroll_y;

        cursor_start_x = mouse_x;
        cursor_start_y = mouse_y;

        text_renderer.setCursorPositions(buffer, cursor_start_x, cursor_start_y, mouse_x, mouse_y,
                                         main_font_rasterizer);
    }

    void onLeftMouseDrag(float mouse_x, float mouse_y) {
        mouse_x -= editor_offset_x;
        mouse_y -= editor_offset_y;
        mouse_x += scroll_x;
        mouse_y += scroll_y;

        text_renderer.setCursorPositions(buffer, cursor_start_x, cursor_start_y, mouse_x, mouse_y,
                                         main_font_rasterizer);
    }

    void onKeyDown(app::Key key, app::ModifierKey modifiers) {
        using app::Any;

        // Detect only `super+w` — no additional modifiers allowed.
        if (key == app::Key::kW && Any(modifiers & app::ModifierKey::kSuper) &&
            !Any(modifiers & ~app::ModifierKey::kSuper)) {
            std::cerr << "close window\n";
        }
    }

private:
    int id;

    float scroll_x = 0;
    float scroll_y = 0;

    float cursor_start_x = 0;
    float cursor_start_y = 0;

    int editor_offset_x = 200 * 2;
    int editor_offset_y = 30 * 2;

    Buffer buffer;
    SyntaxHighlighter highlighter;

    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;

    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    ImageRenderer image_renderer;
};

class SimpleText : public App {
public:
    SimpleText() : editor_window1(0), editor_window2(1) {}

    void onActivate() {
        createNewWindow(editor_window1, 1200, 800);
        createNewWindow(editor_window2, 600, 400);
    }

private:
    EditorWindow editor_window1;
    EditorWindow editor_window2;
};

int SimpleTextMain(int argc, char* argv[]) {
    SimpleText editor;
    editor.run();
    return 0;
}
