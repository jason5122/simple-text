#pragma once

#include <OpenGL/gl3.h>
#include <cstddef>

class TriangleRenderer {
public:
    TriangleRenderer() = default;
    void setup(float width, float height);
    void draw();
    void resize(int new_width, int new_height);
    ~TriangleRenderer();

private:
    static const int BATCH_MAX = 65536;
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    void linkShaders();
};
