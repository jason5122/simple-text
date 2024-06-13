#include "opengl/functionsgl_enums.h"
#include "renderer.h"

#include <format>
#include <iostream>

namespace renderer {

Renderer::Renderer(opengl::FunctionsGL* gl) : gl{gl}, rect_renderer{gl} {}

void Renderer::setup() {
    rect_renderer.setup();

    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // GLuint tex_id;
    // gl->genTextures(1, &tex_id);
    // std::cerr << std::format("tex_id = {}", tex_id) << '\n';
}

void Renderer::draw(const Size& size) {
    gl->viewport(0, 0, size.width, size.height);

    gl->clear(GL_COLOR_BUFFER_BIT);

    // gl->blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    // rect_renderer.draw(size, {0, 0}, {0, 0, 0}, 0, 20 * 2, 50, 1000, {200 * 2, 30 * 2}, 10 * 2);
}

}
