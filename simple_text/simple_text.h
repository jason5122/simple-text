#pragma once

#include "gui/app.h"
#include <vector>

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "renderer/image_renderer.h"
#include "renderer/rect_renderer.h"
#include "renderer/text_renderer.h"

class SimpleText : public App {
public:
    class EditorWindow : public App::Window {
    public:
        int wid;

        EditorWindow(SimpleText& parent, int width, int height, int wid);
        ~EditorWindow();

        void onOpenGLActivate(int width, int height) override;
        void onDraw() override;
        void onResize(int width, int height) override;
        void onScroll(float dx, float dy) override;
        void onLeftMouseDown(float mouse_x, float mouse_y) override;
        void onLeftMouseDrag(float mouse_x, float mouse_y) override;
        void onKeyDown(app::Key key, app::ModifierKey modifiers) override;
        void onClose() override;

    private:
        SimpleText& parent;

        float scroll_x = 0;
        float scroll_y = 0;

        int editor_offset_x = 200 * 2;
        int editor_offset_y = 30 * 2;

        CursorInfo start_cursor{};
        CursorInfo end_cursor{};

        // TODO: Update this during insertion/deletion.
        float longest_line_x = 0;

        Buffer buffer;
        SyntaxHighlighter highlighter;

        TextRenderer text_renderer;
    };

    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;
    RectRenderer rect_renderer;
    ImageRenderer image_renderer;

    SimpleText();
    ~SimpleText();
    void createWindow();
    void destroyWindow(int wid);
    void createNWindows(int n);
    void destroyAllWindows();

    void onLaunch() override;

private:
    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};
