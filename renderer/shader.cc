#include "opengl/functions_gl_enums.h"
#include "shader.h"
#include <format>
#include <iostream>

namespace renderer {

Shader::Shader(std::shared_ptr<opengl::FunctionsGL> shared_gl, const std::string& vert_source,
               const std::string& frag_source)
    : gl{std::move(shared_gl)} {
    const char* vert_source_c = vert_source.c_str();
    const char* frag_source_c = frag_source.c_str();

    GLuint vertex_shader = gl->createShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = gl->createShader(GL_FRAGMENT_SHADER);
    gl->shaderSource(vertex_shader, 1, &vert_source_c, nullptr);
    gl->shaderSource(fragment_shader, 1, &frag_source_c, nullptr);

    GLint success = 0;
    gl->compileShader(vertex_shader);
    gl->getShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_size = 0;
        gl->getShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        gl->getShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("vertex shader: {}", error);

        gl->deleteShader(vertex_shader);
        // TODO: Handle errors in constructor.
    }

    gl->compileShader(fragment_shader);
    gl->getShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_size = 0;
        gl->getShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        gl->getShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("fragment shader: {}", error);

        gl->deleteShader(fragment_shader);
        // TODO: Handle errors in constructor.
    }

    id_ = gl->createProgram();
    gl->attachShader(id_, vertex_shader);
    gl->attachShader(id_, fragment_shader);

    gl->linkProgram(id_);
    gl->getProgramiv(id_, GL_LINK_STATUS, &success);
    if (!success) {
        // TODO: Do this in a more robust way.
        char info_log[512];
        gl->getProgramInfoLog(id_, 512, nullptr, info_log);
        std::cerr << "Shader linking error:\n" << info_log << '\n';

        // TODO: Use RAII wrappers to automatically destruct shaders and program.
        // TODO: Handle errors in constructor.
    }

    gl->deleteShader(vertex_shader);
    gl->deleteShader(fragment_shader);
}

Shader::~Shader() {
    gl->deleteProgram(id_);
}

Shader::Shader(Shader&& other) : id_(other.id_) {
    other.id_ = 0;
}

Shader& Shader::operator=(Shader&& other) {
    if (&other != this) {
        id_ = other.id_;
        other.id_ = 0;
    }
    return *this;
}

GLuint Shader::id() {
    return id_;
}

}
