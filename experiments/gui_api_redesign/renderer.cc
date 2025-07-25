#include "experiments/gui_api_redesign/renderer.h"
#include "gl/gl.h"
#include <spdlog/spdlog.h>

using namespace gl;

Renderer::Renderer() {}

std::unique_ptr<Renderer> Renderer::create() { return std::unique_ptr<Renderer>(new Renderer()); }

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(0.5, 0, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::draw_rect() { spdlog::info("draw_rect"); }

void Renderer::draw_text() { spdlog::info("draw_text"); }
