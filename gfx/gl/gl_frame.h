#pragma once

#include "gfx/frame.h"
#include "gfx/gl/gl_device.h"
#include "gfx/gl/gl_surface.h"
#include "gl/gl.h"
#include <cstddef>
#include <vector>

namespace gfx {

class GLFrame final : public Frame {
public:
    GLFrame(GLDevice& device, GLSurface& surface) : device_(device), surface_(surface) {}

    Device& device() override { return device_; }

    void clear(const Color& c) override;
    void draw_quads(std::span<const Quad> quads, float transform_x, float transform_y) override;
    void draw_textured_quads(std::span<const TexturedQuad> quads,
                             Texture& texture,
                             BlendMode blend,
                             float transform_x,
                             float transform_y) override;
    void finish() override;

private:
    GLDevice& device_;
    GLSurface& surface_;

    // Ordered draw list: commands are replayed in submission order on finish() so callers control
    // z-order by construction. Solid and textured vertices live in separate streams; each command
    // references a range in the stream matching its kind.
    struct Command {
        enum class Kind { kSolid, kTextured };
        Kind kind;
        std::size_t offset;
        std::size_t count;
        gl::GLuint texture;
        BlendMode blend;
    };

    std::vector<GLVertex> solid_vertices_;
    std::vector<GLTexVertex> tex_vertices_;
    std::vector<Command> commands_;
};

}  // namespace gfx
