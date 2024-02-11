#pragma once

#include "ui/renderer/shader.h"
#include <OpenGL/gl3.h>
#include <cstddef>

class RectRenderer {
public:
    RectRenderer() = default;
    void setup(float width, float height);
    void draw(float scroll_x, float scroll_y, float cursor_x, size_t cursor_line,
              float line_height, size_t line_count, float longest_x, size_t visible_lines);
    void resize(int new_width, int new_height);
    ~RectRenderer();

private:
    static const int BATCH_MAX = 65536;
    float width, height;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
