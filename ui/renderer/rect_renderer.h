#pragma once

#include "ui/renderer/shader.h"
#include <cstddef>
#include <glad/glad.h>

class RectRenderer {
public:
    RectRenderer() = default;
    void setup();
    void draw(int width, int height, float scroll_x, float scroll_y, float cursor_x,
              size_t cursor_line, float line_height, size_t line_count, float longest_x,
              float editor_offset_x, float editor_offset_y, float status_bar_height);
    ~RectRenderer();

private:
    static const int BATCH_MAX = 65536;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
