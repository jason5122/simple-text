#pragma once

#include <OpenGL/gl3.h>

class Atlas {
    GLuint tex_id;
    int width;
    int height;
    int row_extent = 0;
    int row_baseline = 0;
    int row_tallest = 0;

public:
    Atlas(int size);
};
