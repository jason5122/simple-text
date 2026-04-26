#include "gfx/gl/gl_frame.h"
#include "gl/gl.h"
using namespace gl;

namespace gfx {

void GLFrame::clear(const Color& c) {
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLFrame::draw_quads(std::span<const Quad> quads, float transform_x, float transform_y) {
    if (quads.empty()) return;

    const int w = surface_.width();
    const int h = surface_.height();
    if (w <= 0 || h <= 0) return;

    // Build 6 vertices per quad (two triangles).
    vertices_.reserve(quads.size() * 6);

    for (const Quad& q : quads) {
        const float x0 = q.x + transform_x;
        const float y0 = q.y + transform_y;
        const float x1 = q.x + q.w + transform_x;
        const float y1 = q.y + q.h + transform_y;

        GLVertex v00 = {x0, y0, q.r, q.g, q.b, q.a};
        GLVertex v10 = {x1, y0, q.r, q.g, q.b, q.a};
        GLVertex v01 = {x0, y1, q.r, q.g, q.b, q.a};
        GLVertex v11 = {x1, y1, q.r, q.g, q.b, q.a};

        // Triangle 1: v00, v10, v01
        vertices_.push_back(v00);
        vertices_.push_back(v10);
        vertices_.push_back(v01);

        // Triangle 2: v10, v11, v01
        vertices_.push_back(v10);
        vertices_.push_back(v11);
        vertices_.push_back(v01);
    }
}

void GLFrame::finish() {
    const int w = surface_.width();
    const int h = surface_.height();

    glViewport(0, 0, w, h);
    device_.draw_solid_quads(vertices_, w, h);

    vertices_.clear();
    glFlush();
}

}  // namespace gfx
