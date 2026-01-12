#include "gfx/gl/gl_frame.h"
#include "gl/gl.h"
#include <spdlog/spdlog.h>
using namespace gl;

namespace gfx {

void GLFrame::clear(const Color& c) {
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLFrame::set_viewport(int width, int height) { glViewport(0, 0, width, height); }

void GLFrame::draw_quads(std::span<const Quad> quads, float transform_x, float transform_y) {
    if (quads.empty()) return;

    if (!device_.init_quad_pipeline()) {
        spdlog::error("GLDevice quad pipeline init failed");
        return;
    }

    const int w = surface_.width();
    const int h = surface_.height();
    if (w <= 0 || h <= 0) return;

    // Build 6 vertices per quad (two triangles).
    scratch_.clear();
    scratch_.reserve(quads.size() * 6);

    for (const Quad& q : quads) {
        const float x0 = q.x;
        const float y0 = q.y;
        const float x1 = q.x + q.w;
        const float y1 = q.y + q.h;

        Vertex v00 = {x0, y0, q.r, q.g, q.b, q.a};
        Vertex v10 = {x1, y0, q.r, q.g, q.b, q.a};
        Vertex v01 = {x0, y1, q.r, q.g, q.b, q.a};
        Vertex v11 = {x1, y1, q.r, q.g, q.b, q.a};

        // Triangle 1: v00, v10, v01
        scratch_.push_back(v00);
        scratch_.push_back(v10);
        scratch_.push_back(v01);

        // Triangle 2: v10, v11, v01
        scratch_.push_back(v10);
        scratch_.push_back(v11);
        scratch_.push_back(v01);
    }

    glUseProgram(device_.quad_program());
    glUniform2f(device_.quad_u_viewport_loc(), (float)w, (float)h);
    glUniform2f(device_.pipeline_.u_translate_loc, transform_x, transform_y);

    glBindVertexArray(device_.quad_vao());
    glBindBuffer(GL_ARRAY_BUFFER, device_.quad_vbo());

    // Upload vertices. STREAM_DRAW is appropriate for per-frame updates.
    std::span<const Vertex> verts{scratch_};
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size_bytes()), verts.data(),
                 GL_STREAM_DRAW);

    // Blend for alpha.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)scratch_.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GLFrame::present() { glFlush(); }

}  // namespace gfx
