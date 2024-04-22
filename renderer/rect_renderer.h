#pragma once

#include "renderer/shader.h"
#include "renderer/types.h"
#include <cstddef>
#include <glad/glad.h>

namespace renderer {
class RectRenderer {
public:
    RectRenderer();
    ~RectRenderer();
    void setup();
    void draw(int width, int height, float scroll_x, float scroll_y, CursorInfo& end_cursor,
              float line_height, size_t line_count, float longest_x, float editor_offset_x,
              float editor_offset_y, float status_bar_height);

private:
    static const int BATCH_MAX = 65536;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
}
