#pragma once

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

class AtlasRenderer {
public:
    AtlasRenderer() = default;
    void setup(float width, float height);
    void draw(float x, float y, GLuint atlas);
    void resize(int new_width, int new_height);
    ~AtlasRenderer();

private:
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo, ebo;

    void linkShaders();
};
