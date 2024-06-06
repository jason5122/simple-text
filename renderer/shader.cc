#include "shader.h"
#include <vector>

#include <format>
#include <iostream>

Shader::~Shader() {
    glDeleteProgram(id);
}

bool Shader::link(fs::path vert_path, fs::path frag_path) {
    std::string vert_source = ReadFile(vert_path);
    std::string frag_source = ReadFile(frag_path);
    const char* vert_source_c = vert_source.c_str();
    const char* frag_source_c = frag_source.c_str();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex_shader, 1, &vert_source_c, nullptr);
    glShaderSource(fragment_shader, 1, &frag_source_c, nullptr);

    GLint success = 0;
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_size = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<GLchar> error_log(log_size);
        glGetShaderInfoLog(vertex_shader, log_size, &log_size, &error_log[0]);
        fprintf(stderr, "vertex shader: %s", &error_log[0]);

        glDeleteShader(vertex_shader);
        return false;
    }

    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_size = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<GLchar> error_log(log_size);
        glGetShaderInfoLog(fragment_shader, log_size, &log_size, &error_log[0]);
        fprintf(stderr, "fragment shader: %s", &error_log[0]);

        glDeleteShader(fragment_shader);
        return false;
    }

    id = glCreateProgram();
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    glLinkProgram(id);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return true;
}

bool Shader::link(const std::string& vert_source, const std::string& frag_source) {
    const char* vert_source_c = vert_source.c_str();
    const char* frag_source_c = frag_source.c_str();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex_shader, 1, &vert_source_c, nullptr);
    glShaderSource(fragment_shader, 1, &frag_source_c, nullptr);

    GLint success = 0;
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_size = 0;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        glGetShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("vertex shader: {}", error);

        glDeleteShader(vertex_shader);
        return false;
    }

    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_size = 0;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string error;
        error.reserve(log_size);
        glGetShaderInfoLog(vertex_shader, log_size, nullptr, &error[0]);
        std::cerr << std::format("fragment shader: {}", error);

        glDeleteShader(fragment_shader);
        return false;
    }

    id = glCreateProgram();
    glAttachShader(id, vertex_shader);
    glAttachShader(id, fragment_shader);
    glLinkProgram(id);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return true;
}
