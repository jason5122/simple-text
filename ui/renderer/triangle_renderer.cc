#include "triangle_renderer.h"
#include <fstream>

void TriangleRenderer::setup() {
    this->linkShaders();

    float vertices[] = {
        0.5f,  -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        0.0f,  0.5f,  0.0f   // top
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void TriangleRenderer::draw() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao);

    glUseProgram(shader_program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

inline const char* ReadFileCpp(const char* file_name) {
    std::ifstream in(file_name);
    static std::string contents((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
    return contents.c_str();
}

void TriangleRenderer::linkShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFileCpp("shaders/triangle_vert.glsl");
    const GLchar* frag_source = ReadFileCpp("shaders/triangle_frag.glsl");

    glShaderSource(vertex_shader, 1, &vert_source, nullptr);
    glShaderSource(fragment_shader, 1, &frag_source, nullptr);
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}
