#import "AtlasRenderer.h"
#import "ui/renderer/Atlas.h"
#import "util/FileUtil.h"

AtlasRenderer::AtlasRenderer(float width, float height) {
    this->linkShaders();
    this->resize(width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void AtlasRenderer::draw(float x, float y, GLuint atlas) {
    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    float w = Atlas::ATLAS_SIZE;
    float h = Atlas::ATLAS_SIZE;

    float vertices[4][4] = {
        {x + w, y + h, 1.0f, 0.0f},  // bottom right
        {x + w, y, 1.0f, 1.0f},      // top right
        {x, y, 0.0f, 1.0f},          // top left
        {x, y + h, 0.0f, 0.0f},      // bottom left
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glBindTexture(GL_TEXTURE_2D, atlas);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void AtlasRenderer::resize(int new_width, int new_height) {
    width = new_width;
    height = new_height;

    glViewport(0, 0, width, height);
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);
}

void AtlasRenderer::linkShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFile(ResourcePath("shaders/atlas_vert.glsl"));
    const GLchar* frag_source = ReadFile(ResourcePath("shaders/atlas_frag.glsl"));
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

AtlasRenderer::~AtlasRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
}
