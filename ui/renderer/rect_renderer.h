#pragma once

#include "ui/renderer/shader.h"
#include <cstddef>

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

class RectRenderer {
public:
    RectRenderer() = default;
    void setup(float width, float height);
    void draw(float scroll_x, float scroll_y, float cursor_x, size_t cursor_line,
              float line_height, size_t line_count, float longest_x, float editor_offset_x,
              float editor_offset_y);
    void resize(int new_width, int new_height);
    ~RectRenderer();

private:
    static const int BATCH_MAX = 65536;
    float width, height;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
