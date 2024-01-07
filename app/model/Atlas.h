#pragma once

#include "model/Rasterizer.h"
#include <OpenGL/gl3.h>

struct Glyph {
    GLuint tex_id;
    int16_t top;
    int16_t left;
    int16_t width;
    int16_t height;
    float uv_bot;
    float uv_left;
    float uv_width;
    float uv_height;
};

class Atlas {
    GLuint tex_id;
    int width;
    int height;
    int row_extent = 0;
    int row_baseline = 0;
    int row_tallest = 0;

public:
    Atlas(int32_t size);
    Glyph insert_inner(RasterizedGlyph& glyph);
};
