#include "gfx/gl/gl_device.h"
#include "gfx/gl/gl_surface.h"
#include "gl/loader.h"
#include <spdlog/spdlog.h>
using namespace gl;

namespace gfx {

std::unique_ptr<Surface> GLDevice::create_surface(int width, int height) {
    if (!initialized_) {
        gl::load_global_function_pointers();
        initialized_ = true;
    }

    return std::make_unique<GLSurface>(*this, width, height);
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
    GLint translate_loc = glGetUniformLocation(prog, "u_translate_px");
    if (translate_loc < 0) {
        spdlog::error("GL uniform not found: u_translate_px");
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex),
                          (void*)offsetof(Vertex, x));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex),
                          (void*)offsetof(Vertex, r));

    // Unbind to keep state tidy.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    pipeline_.program = prog;
    pipeline_.vao = vao;
    pipeline_.vbo = vbo;
    pipeline_.u_viewport_loc = viewport_loc;
    pipeline_.u_translate_loc = translate_loc;
    pipeline_.initialized = true;
    return true;
}

}  // namespace gfx
