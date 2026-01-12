#pragma once

#include "gfx/device.h"
#include "gl/gl.h"

namespace gfx {

struct Vertex {
    float x, y;
    float r, g, b, a;
};

class GLDevice final : public Device {
public:
    std::unique_ptr<Surface> create_surface(int width, int height) override;

    // TODO: Clean OpenGL stuff up.
    bool init_quad_pipeline();
    gl::GLuint quad_program() const { return pipeline_.program; }
    gl::GLuint quad_vao() const { return pipeline_.vao; }
    gl::GLuint quad_vbo() const { return pipeline_.vbo; }
    gl::GLint quad_u_viewport_loc() const { return pipeline_.u_viewport_loc; }

private:
    friend class GLFrame;

    bool initialized_ = false;

    // TODO: Clean OpenGL stuff up.
    struct QuadPipeline {
        bool initialized = false;
        gl::GLuint program = 0;
        gl::GLuint vao = 0;
        gl::GLuint vbo = 0;
        gl::GLint u_viewport_loc = -1;
        gl::GLint u_translate_loc = -1;
    };
    QuadPipeline pipeline_;

    static gl::GLuint compile_shader(gl::GLenum type, const char* src);
    static gl::GLuint link_program(gl::GLuint vs, gl::GLuint fs);
};

}  // namespace gfx
