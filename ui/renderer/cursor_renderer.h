#pragma once

#include <OpenGL/gl3.h>
#include <cstddef>

class CursorRenderer {
public:
    CursorRenderer(float width, float height);
    void draw(float scroll_x, float scroll_y, float cursor_x, size_t cursor_line,
              float line_height, size_t line_count, size_t visible_lines, float longest_x);
    void resize(int new_width, int new_height);
    ~CursorRenderer();

private:
    static const int BATCH_MAX = 65536;
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    void linkShaders();
};
