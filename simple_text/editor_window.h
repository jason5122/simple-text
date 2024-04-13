#pragma once

#include "simple_text/simple_text.h"
#include "ui/app/app.h"

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"

class EditorWindow : public App::Window {
public:
    EditorWindow(SimpleText& parent, int width, int height);
    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onScroll(float dx, float dy) override;
    void onLeftMouseDown(float mouse_x, float mouse_y) override;
    void onLeftMouseDrag(float mouse_x, float mouse_y) override;
    void onKeyDown(app::Key key, app::ModifierKey modifiers) override;

private:
    SimpleText& parent;

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
