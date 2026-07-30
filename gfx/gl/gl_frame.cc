#include "gfx/gl/gl_frame.h"
#include "gfx/gl/gl_texture.h"
#include "gl/gl.h"
using namespace gl;

namespace gfx {

void GLFrame::clear(const Color& c) {
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLFrame::draw_quads(std::span<const Quad> quads, float transform_x, float transform_y) {
    if (quads.empty()) return;

    const std::size_t offset = solid_vertices_.size();
    solid_vertices_.reserve(offset + quads.size() * 6);

    for (const Quad& q : quads) {
        const float x0 = q.x + transform_x;
        const float y0 = q.y + transform_y;
        const float x1 = q.x + q.w + transform_x;
        const float y1 = q.y + q.h + transform_y;

        GLVertex v00 = {x0, y0, q.r, q.g, q.b, q.a};
        GLVertex v10 = {x1, y0, q.r, q.g, q.b, q.a};
        GLVertex v01 = {x0, y1, q.r, q.g, q.b, q.a};
        GLVertex v11 = {x1, y1, q.r, q.g, q.b, q.a};

        solid_vertices_.push_back(v00);
        solid_vertices_.push_back(v10);
        solid_vertices_.push_back(v01);
        solid_vertices_.push_back(v10);
        solid_vertices_.push_back(v11);
        solid_vertices_.push_back(v01);
    }

    commands_.push_back(
        {Command::Kind::kSolid, offset, quads.size() * 6, 0, BlendMode::kAlpha});
}

void GLFrame::draw_textured_quads(std::span<const TexturedQuad> quads,
                                  Texture& texture,
                                  BlendMode blend,
                                  float transform_x,
                                  float transform_y) {
    if (quads.empty()) return;

    auto& gl_texture = static_cast<GLTexture&>(texture);

    const std::size_t offset = tex_vertices_.size();
    tex_vertices_.reserve(offset + quads.size() * 6);

    for (const TexturedQuad& q : quads) {
        const float x0 = q.x + transform_x;
        const float y0 = q.y + transform_y;
        const float x1 = q.x + q.w + transform_x;
        const float y1 = q.y + q.h + transform_y;

        GLTexVertex v00 = {x0, y0, q.u0, q.v0, q.r, q.g, q.b, q.a};
        GLTexVertex v10 = {x1, y0, q.u1, q.v0, q.r, q.g, q.b, q.a};
        GLTexVertex v01 = {x0, y1, q.u0, q.v1, q.r, q.g, q.b, q.a};
        GLTexVertex v11 = {x1, y1, q.u1, q.v1, q.r, q.g, q.b, q.a};

        tex_vertices_.push_back(v00);
        tex_vertices_.push_back(v10);
        tex_vertices_.push_back(v01);
        tex_vertices_.push_back(v10);
        tex_vertices_.push_back(v11);
        tex_vertices_.push_back(v01);
    }

    commands_.push_back(
        {Command::Kind::kTextured, offset, quads.size() * 6, gl_texture.id(), blend});
}

void GLFrame::finish() {
    const int w = surface_.width();
    const int h = surface_.height();

    glViewport(0, 0, w, h);

    for (const Command& cmd : commands_) {
        switch (cmd.kind) {
            case Command::Kind::kSolid:
                device_.draw_solid_quads(
                    std::span(solid_vertices_).subspan(cmd.offset, cmd.count), w, h, cmd.blend);
                break;
            case Command::Kind::kTextured:
                device_.draw_textured_quads(
                    std::span(tex_vertices_).subspan(cmd.offset, cmd.count), cmd.texture, w, h,
                    cmd.blend);
                break;
        }
    }

    solid_vertices_.clear();
    tex_vertices_.clear();
    commands_.clear();
    glFlush();
}

}  // namespace gfx
