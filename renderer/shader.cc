#include "opengl/functionsgl_enums.h"
#include "shader.h"
#include <format>
#include <iostream>

namespace renderer {

Shader::Shader(opengl::FunctionsGL* gl) : gl{gl} {}

Shader::~Shader() {
    gl->deleteProgram(tex_id_);
}

bool Shader::link(const std::string& vert_source, const std::string& frag_source) {
    const char* vert_source_c = vert_source.c_str();
    const char* frag_source_c = frag_source.c_str();

    GLuint vertex_shader = gl->createShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = gl->createShader(GL_FRAGMENT_SHADER);
    gl->shaderSource(vertex_shader, 1, &vert_source_c, nullptr);
    gl->shaderSource(fragment_shader, 1, &frag_source_c, nullptr);

    GLint success = 0;
    gl->compileShader(vertex_shader);
    gl->getShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_size = 0;
        gl->getShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        gl->getShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("vertex shader: {}", error);

        gl->deleteShader(vertex_shader);
        return false;
    }

    gl->compileShader(fragment_shader);
    gl->getShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_size = 0;
        gl->getShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        gl->getShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("fragment shader: {}", error);

        gl->deleteShader(fragment_shader);
        return false;
    }

    tex_id_ = gl->createProgram();
    gl->attachShader(tex_id_, vertex_shader);
    gl->attachShader(tex_id_, fragment_shader);
    gl->linkProgram(tex_id_);

    gl->deleteShader(vertex_shader);
    gl->deleteShader(fragment_shader);
    return true;
}

}
