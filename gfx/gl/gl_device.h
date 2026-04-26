#pragma once

#include "gfx/device.h"
#include "gl/gl.h"
#include <span>

namespace gfx {

struct GLVertex {
    float x, y;
    float r, g, b, a;
};

class GLDevice final : public Device {
public:
    std::unique_ptr<Surface> create_surface(int width, int height) override;

    void draw_solid_quads(std::span<const GLVertex> vertices,
                          int viewport_width,
                          int viewport_height);

private:
    bool initialized_ = false;

    struct QuadPipeline {
        bool initialized = false;
        gl::GLuint program = 0;
        gl::GLuint vao = 0;
        gl::GLuint vbo = 0;
        gl::GLint u_viewport_loc = -1;
    };
    QuadPipeline pipeline_;

    bool init_quad_pipeline();

    static gl::GLuint compile_shader(gl::GLenum type, const char* src);
    static gl::GLuint link_program(gl::GLuint vs, gl::GLuint fs);
};

}  // namespace gfx
