#include "gfx/gl/gl_device.h"
#include "gfx/gl/gl_surface.h"
#include "gfx/gl/gl_texture.h"
#include <spdlog/spdlog.h>
using namespace gl;

namespace gfx {

namespace {

void apply_blend(BlendMode blend) {
    glEnable(GL_BLEND);
    switch (blend) {
    case BlendMode::kAlpha:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BlendMode::kPremultiplied:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

}  // namespace

std::unique_ptr<Surface> GLDevice::create_surface(int width, int height) {
    return std::make_unique<GLSurface>(*this, width, height);
}

std::unique_ptr<Texture> GLDevice::create_texture(int width,
                                                  int height,
                                                  TextureFormat format,
                                                  std::span<const uint8_t> pixels) {
    return std::make_unique<GLTexture>(width, height, format, pixels);
}

GLuint GLDevice::compile_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        GLsizei n = 0;
        glGetShaderInfoLog(sh, (GLsizei)sizeof(log), &n, log);
        spdlog::error("GL shader compile failed: {}", std::string(log, (size_t)n));
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint GLDevice::link_program(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        GLsizei n = 0;
        glGetProgramInfoLog(prog, (GLsizei)sizeof(log), &n, log);
        spdlog::error("GL program link failed: {}", std::string(log, (size_t)n));
        glDeleteProgram(prog);
        return 0;
    }

    glDetachShader(prog, vs);
    glDetachShader(prog, fs);
    return prog;
}

bool GLDevice::init_quad_pipeline() {
    if (pipeline_.initialized) return true;

    // Requires a current GL context and loaded function pointers.
    const char* kQuadVS =
#include "gfx/gl/quad_vert.glsl"
        ;
    const char* kQuadFS =
#include "gfx/gl/quad_frag.glsl"
        ;

    GLuint vs = compile_shader(GL_VERTEX_SHADER, kQuadVS);
    if (!vs) return false;

    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, kQuadFS);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    GLuint prog = link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) return false;

    GLint viewport_loc = glGetUniformLocation(prog, "u_viewport_px");
    if (viewport_loc < 0) {
        spdlog::error("GL uniform not found: u_viewport_px");
        glDeleteProgram(prog);
        return false;
    }

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Vertex layout: vec2 pos @ location 0, vec4 color @ location 1
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLVertex),
                          (void*)offsetof(GLVertex, x));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLVertex),
                          (void*)offsetof(GLVertex, r));

    // Unbind to keep state tidy.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    pipeline_.program = prog;
    pipeline_.vao = vao;
    pipeline_.vbo = vbo;
    pipeline_.u_viewport_loc = viewport_loc;
    pipeline_.initialized = true;
    return true;
}

void GLDevice::draw_solid_quads(std::span<const GLVertex> vertices,
                                int viewport_width,
                                int viewport_height,
                                BlendMode blend) {
    if (vertices.empty()) return;
    if (viewport_width <= 0 || viewport_height <= 0) return;

    if (!init_quad_pipeline()) {
        spdlog::error("GLDevice quad pipeline init failed");
        return;
    }

    glUseProgram(pipeline_.program);
    glUniform2f(pipeline_.u_viewport_loc, (float)viewport_width, (float)viewport_height);

    glBindVertexArray(pipeline_.vao);
    glBindBuffer(GL_ARRAY_BUFFER, pipeline_.vbo);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size_bytes()), vertices.data(),
                 GL_STREAM_DRAW);

    apply_blend(blend);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

bool GLDevice::init_tex_pipeline() {
    if (tex_pipeline_.initialized) return true;

    const char* kTexVS =
#include "gfx/gl/tex_vert.glsl"
        ;
    const char* kTexFS =
#include "gfx/gl/tex_frag.glsl"
        ;

    GLuint vs = compile_shader(GL_VERTEX_SHADER, kTexVS);
    if (!vs) return false;

    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, kTexFS);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    GLuint prog = link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) return false;

    GLint viewport_loc = glGetUniformLocation(prog, "u_viewport_px");
    if (viewport_loc < 0) {
        spdlog::error("GL uniform not found: u_viewport_px");
        glDeleteProgram(prog);
        return false;
    }
    GLint tex_loc = glGetUniformLocation(prog, "u_tex");
    if (tex_loc < 0) {
        spdlog::error("GL uniform not found: u_tex");
        glDeleteProgram(prog);
        return false;
    }

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Vertex layout: vec2 pos @ 0, vec2 uv @ 1, vec4 color @ 2.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLTexVertex),
                          (void*)offsetof(GLTexVertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLTexVertex),
                          (void*)offsetof(GLTexVertex, u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLTexVertex),
                          (void*)offsetof(GLTexVertex, r));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    tex_pipeline_.program = prog;
    tex_pipeline_.vao = vao;
    tex_pipeline_.vbo = vbo;
    tex_pipeline_.u_viewport_loc = viewport_loc;
    tex_pipeline_.u_tex_loc = tex_loc;
    tex_pipeline_.initialized = true;
    return true;
}

void GLDevice::draw_textured_quads(std::span<const GLTexVertex> vertices,
                                   GLuint texture,
                                   int viewport_width,
                                   int viewport_height,
                                   BlendMode blend) {
    if (vertices.empty()) return;
    if (viewport_width <= 0 || viewport_height <= 0) return;

    if (!init_tex_pipeline()) {
        spdlog::error("GLDevice textured pipeline init failed");
        return;
    }

    glUseProgram(tex_pipeline_.program);
    glUniform2f(tex_pipeline_.u_viewport_loc, (float)viewport_width, (float)viewport_height);
    glUniform1i(tex_pipeline_.u_tex_loc, 0);

    glBindVertexArray(tex_pipeline_.vao);
    glBindBuffer(GL_ARRAY_BUFFER, tex_pipeline_.vbo);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size_bytes()), vertices.data(),
                 GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    apply_blend(blend);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

}  // namespace gfx
