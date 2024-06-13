#include "opengl/functionsgl_enums.h"
#include "renderer.h"

#include <format>
#include <iostream>

namespace renderer {

Renderer::Renderer(opengl::FunctionsGL* gl) : gl{gl}, rect_renderer{gl} {}

void Renderer::setup() {
    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(0.5f, 0.0f, 0.0f, 1.0f);

    GLuint tex_id;
    gl->genTextures(1, &tex_id);
    std::cerr << std::format("tex_id = {}", tex_id) << '\n';
}

void Renderer::draw() {
    gl->clear(GL_COLOR_BUFFER_BIT);
}

}
