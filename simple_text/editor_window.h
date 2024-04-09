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
    EditorWindow(App& app, int idx) : App::Window(app), idx(idx) {}

private:
    int idx;

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

    void onOpenGLActivateVirtual(int width, int height);
    void onDrawVirtual();
    void onResizeVirtual(int width, int height);
    void onScrollVirtual(float dx, float dy);
    void onLeftMouseDownVirtual(float mouse_x, float mouse_y);
    void onLeftMouseDragVirtual(float mouse_x, float mouse_y);
    void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers);
};
