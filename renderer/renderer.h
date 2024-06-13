#pragma once

#include "opengl/functions_gl.h"
#include "renderer/rect_renderer.h"
#include "renderer/text/glyph_cache.h"

namespace renderer {

class Renderer {
public:
    Renderer(opengl::FunctionsGL* gl);

    void setup();
    void draw(const Size& size);

private:
    opengl::FunctionsGL* gl;

    GlyphCache main_glyph_cache;
    RectRenderer rect_renderer;
};

}
