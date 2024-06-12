#include "opengl/functionsgl_enums.h"
#include "renderer.h"

#include <iostream>

namespace renderer {

Renderer::Renderer(opengl::FunctionsGL* gl) : gl(gl) {}

void Renderer::setup() {
    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(0.5f, 0.0f, 0.0f, 1.0f);

    GLuint tex_id;
    gl->genTextures(1, &tex_id);
    std::cerr << std::format("tex_id = {}", tex_id) << '\n';
}

void Renderer::draw() {
    GLenum err;
    while ((err = gl->getError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error\n";
    }

    gl->clear(GL_COLOR_BUFFER_BIT);
}

}
