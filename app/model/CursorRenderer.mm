#import "CursorRenderer.h"
#import "model/Atlas.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"

CursorRenderer::CursorRenderer(float width, float height) {
    this->linkShaders();
    this->resize(width, height);

    glDepthMask(GL_FALSE);

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, nullptr, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CursorRenderer::draw(float x_old, float y_old, float cell_width, float cell_height) {
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "cell_dim"), cell_width, cell_height);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // float w = 4.0;
    // float h = cell_height;

    float pos_x = 10 * (cell_width + 1);
    float pos_y = 10 * (cell_height + 1);

    float half_width = width / 2;
    float half_height = height / 2;
    float x = pos_x / half_width - 1.0;
    float y = -pos_y / half_height + 1.0;
    // float w = cell_width / half_width;
    // float h = cell_height / half_height;
    float w = 4.0 / half_width;
    float h = cell_height / half_height;

    LogDefault(@"CursorRenderer", @"pos_x = %f, pos_y = %f", pos_x, pos_y);
    LogDefault(@"CursorRenderer", @"width = %f, height = %f", w, h);

    float vertices[4][2] = {
        {x + w, y + h},  // bottom right
        {x + w, y},      // top right
        {x, y},          // top left
        {x, y + h},      // bottom left
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
}

void CursorRenderer::resize(int new_width, int new_height) {
    width = new_width;
    height = new_height;

    glViewport(0, 0, width, height);
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);
}

void CursorRenderer::linkShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFile(ResourcePath("cursor_vert.glsl"));
    const GLchar* frag_source = ReadFile(ResourcePath("cursor_frag.glsl"));
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

CursorRenderer::~CursorRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
}
