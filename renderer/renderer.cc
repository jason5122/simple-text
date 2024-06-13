#include "opengl/functionsgl_enums.h"
#include "renderer.h"

namespace renderer {

Renderer::Renderer(opengl::FunctionsGL* gl)
    : gl{gl}, main_glyph_cache{gl, "Source Code Pro", 16 * 2}, rect_renderer{gl} {}

void Renderer::setup() {
    rect_renderer.setup();

    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // gl->clearColor(1.0f, 0.0f, 1.0f, 1.0f);
}

void Renderer::draw(const Size& size) {
    gl->viewport(0, 0, size.width, size.height);

    gl->clear(GL_COLOR_BUFFER_BIT);

    gl->blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, {0, 0}, {0, 0, 0}, 0, 20 * 2, 50, 1000, {200 * 2, 30 * 2}, 10 * 2);
}

}
