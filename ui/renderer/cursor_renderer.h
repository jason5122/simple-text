#pragma once

#include <OpenGL/gl3.h>

class CursorRenderer {
public:
    CursorRenderer(float width, float height);
    void draw(float scroll_x, float scroll_y, float x, float y, float cell_height);
    void resize(int new_width, int new_height);
    ~CursorRenderer();

private:
    static const int BATCH_MAX = 65536;
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    void linkShaders();
};
