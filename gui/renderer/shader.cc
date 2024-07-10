#include "shader.h"

using namespace opengl;

// TODO: Debug; remove this.
#include <format>
#include <iostream>

namespace gui {

Shader::Shader(const std::string& vert_source, const std::string& frag_source) {
    const char* vert_source_c = vert_source.c_str();
    const char* frag_source_c = frag_source.c_str();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex_shader, 1, &vert_source_c, nullptr);
    glShaderSource(fragment_shader, 1, &frag_source_c, nullptr);

    GLint success = 0;
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_size = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        glGetShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("vertex shader: {}", error);

        glDeleteShader(vertex_shader);
        // TODO: Handle errors in constructor.
    }

    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_size = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        glGetShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("fragment shader: {}", error);

        glDeleteShader(fragment_shader);
        // TODO: Handle errors in constructor.
    }

    id_ = glCreateProgram();
    glAttachShader(id_, vertex_shader);
    glAttachShader(id_, fragment_shader);

    glLinkProgram(id_);
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    if (!success) {
        // TODO: Do this in a more robust way.
        char info_log[512];
        glGetProgramInfoLog(id_, 512, nullptr, info_log);
        std::cerr << "Shader linking error:\n" << info_log << '\n';

        // TODO: Use RAII wrappers to automatically destruct shaders and program.
        // TODO: Handle errors in constructor.
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Shader::~Shader() {
    glDeleteProgram(id_);
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
