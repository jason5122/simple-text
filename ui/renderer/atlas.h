#pragma once

#include "font/types/rasterized_glyph.h"
#include <OpenGL/gl3.h>

struct AtlasGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    float advance;
    float uv_left;
    float uv_bot;
    float uv_width;
    float uv_height;
};

class Atlas {
public:
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    static const int ATLAS_SIZE = 1024;  // 1024 is a conservative size.

    GLuint tex_id;

    Atlas();
    AtlasGlyph insertGlyph(RasterizedGlyph& glyph);

private:
    int offset_x = 0;
    int offset_y = 0;
    int tallest = 0;
};
