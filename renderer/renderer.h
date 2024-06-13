#pragma once

#include "opengl/functions_gl.h"
#include "renderer/rect_renderer.h"

namespace renderer {

class Renderer {
public:
    Renderer(opengl::FunctionsGL* gl);

    void setup();
    void draw();

private:
    opengl::FunctionsGL* gl;

    RectRenderer rect_renderer;
};

}
