#include "editor_window.h"
#include "simple_text/editor_app.h"

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window(parent), wid(wid), parent(parent), color_scheme(isDarkMode()) {
    buffer.setContents(R"(#pragma once

#include "opengl/functions_gl.h"
#include "renderer/rect_renderer.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/text/text_renderer.h"

namespace renderer {

class Renderer {
public:
    Renderer(opengl::FunctionsGL* gl);

    void setup();
    void draw(const Size& size, const base::Buffer& buffer);
    void scroll(int dx, int dy);

private:
    opengl::FunctionsGL* gl;

    GlyphCache main_glyph_cache;
    GlyphCache ui_glyph_cache;
    TextRenderer text_renderer;
    RectRenderer rect_renderer;

    Point editor_offset = {200 * 2, 30 * 2};
    Point scroll_offset{};
};

}
)");
}

void EditorWindow::onOpenGLActivate(int width, int height) {}

void EditorWindow::onDraw(int width, int height) {
    parent.renderer->draw({width, height}, buffer, scroll_offset);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    // scroll_offset.x += dx;
    scroll_offset.y += dy;
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
