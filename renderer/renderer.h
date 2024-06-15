#pragma once

#include "opengl/functions_gl.h"
#include "renderer/movement.h"
#include "renderer/rect_renderer.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/text/text_renderer.h"

namespace renderer {

class Renderer {
public:
    Renderer(opengl::FunctionsGL* gl);

    void draw(const Size& size, const base::Buffer& buffer, const Point& scroll_offset,
              const CaretInfo& end_caret);

    // TODO: Combine this with TextRenderer (and rename TextRenderer).
    Movement movement;

private:
    opengl::FunctionsGL* gl;

    GlyphCache main_glyph_cache;
    GlyphCache ui_glyph_cache;
    TextRenderer text_renderer;
    RectRenderer rect_renderer;

    Point editor_offset{200 * 2, 30 * 2};
};

}
