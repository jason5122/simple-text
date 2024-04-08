#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "ui/app/app.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"

class EditorWindow : public App::Window {
public:
    EditorWindow(App& app, int id) : App::Window(app), id(id) {}

    void onOpenGLActivate(int width, int height);
    void onDraw();
    void onResize(int width, int height);
    void onScroll(float dx, float dy);
    void onLeftMouseDown(float mouse_x, float mouse_y);
    void onLeftMouseDrag(float mouse_x, float mouse_y);
    void onKeyDown(app::Key key, app::ModifierKey modifiers);

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
