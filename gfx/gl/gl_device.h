#pragma once

#include "gfx/device.h"
#include "gfx/frame.h"
#include "gl/gl.h"
#include <cstdint>
#include <memory>
#include <span>

namespace gfx {

struct GLVertex {
    float x, y;
    float r, g, b, a;
};

struct GLTexVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

class GLDevice final : public Device {
public:
    std::unique_ptr<Surface> create_surface(int width, int height) override;
    std::unique_ptr<Texture> create_texture(int width,
                                            int height,
                                            TextureFormat format,
                                            std::span<const uint8_t> pixels = {}) override;

    void draw_solid_quads(std::span<const GLVertex> vertices,
                          int viewport_width,
                          int viewport_height,
                          BlendMode blend);

    void draw_textured_quads(std::span<const GLTexVertex> vertices,
                             gl::GLuint texture,
                             int viewport_width,
                             int viewport_height,
                             BlendMode blend);

private:
    struct QuadPipeline {
        bool initialized = false;
        gl::GLuint program = 0;
        gl::GLuint vao = 0;
        gl::GLuint vbo = 0;
        gl::GLint u_viewport_loc = -1;
    };
    QuadPipeline pipeline_;

    struct TexPipeline {
        bool initialized = false;
        gl::GLuint program = 0;
        gl::GLuint vao = 0;
        gl::GLuint vbo = 0;
        gl::GLint u_viewport_loc = -1;
        gl::GLint u_tex_loc = -1;
    };
    TexPipeline tex_pipeline_;

    bool init_quad_pipeline();
    bool init_tex_pipeline();

    static gl::GLuint compile_shader(gl::GLenum type, const char* src);
    static gl::GLuint link_program(gl::GLuint vs, gl::GLuint fs);
};

}  // namespace gfx
