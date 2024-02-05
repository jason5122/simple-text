#include "base/rgb.h"
#include "cursor_renderer.h"
#include "ui/renderer/atlas.h"
#include "util/file_util.h"
#include "util/log_util.h"

struct InstanceData {
    // Coordinates.
    float coord_x;
    float coord_y;
    // Rectangle size.
    float rect_width;
    float rect_height;
    // Color.
    float r;
    float g;
    float b;
    float a;
};

CursorRenderer::CursorRenderer(float width, float height) {
    this->linkShaders();
    this->resize(width, height);

    glDepthMask(GL_FALSE);

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * BATCH_MAX, nullptr, GL_STATIC_DRAW);

    size_t size = 0;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(0, 1);
    size += 2 * sizeof(float);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(1, 1);
    size += 2 * sizeof(float);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(2, 1);
    size += 4 * sizeof(float);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CursorRenderer::draw(float scroll_x, float scroll_y, float cursor_x, size_t cursor_line,
                          float line_height, size_t line_count, float longest_x) {
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "scroll_offset"), scroll_x, scroll_y);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    float rect_width = 4;
    float rect_height = line_height;

    cursor_x -= rect_width / 2;

    int extra_padding = 8;
    float cursor_y = cursor_line * line_height;
    cursor_y -= extra_padding;
    rect_height += extra_padding * 2;

    std::vector<InstanceData> instances;
    instances.push_back(InstanceData{
        // Coordinates.
        cursor_x + scroll_x,
        cursor_y + scroll_y,
        // Rectangle size.
        rect_width,
        rect_height,
        // Color.
        BLUE2.r,
        BLUE2.g,
        BLUE2.b,
        1.0,
    });

    line_count -= 1;  // TODO: Merge this with EditorView.

    // Add vertical scroll bar.
    float scroll_bar_width = 20;
    float total_y = line_count * line_height;
    float scroll_bar_height = height * (height / total_y);
    float scroll_bar_position_percentage = -scroll_y / (line_count * line_height);
    instances.push_back(InstanceData{
        // Coordinates.
        width - scroll_bar_width,
        (height - scroll_bar_height) * scroll_bar_position_percentage,
        // Rectangle size.
        scroll_bar_width,
        scroll_bar_height,
        // Color.
        182,
        182,
        182,
        1.0,
    });

    // Add horizontal scroll bar.
    instances.push_back(InstanceData{
        // Coordinates.
        width * (-scroll_x / longest_x),
        height - 20,
        // Rectangle size.
        width * (width / longest_x),
        20,
        // Color.
        182,
        182,
        182,
        1.0,
    });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

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
    const GLchar* vert_source = ReadFile(ResourcePath("shaders/cursor_vert.glsl"));
    const GLchar* frag_source = ReadFile(ResourcePath("shaders/cursor_frag.glsl"));
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
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
}
