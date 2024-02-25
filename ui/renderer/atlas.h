#pragma once

#include "font/types/rasterized_glyph.h"
#include "ui/renderer/opengl_types.h"

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

class Atlas {
public:
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    static const int ATLAS_SIZE = 1024;  // 1024 is a conservative size.

    GLuint tex_id;

    Atlas() = default;
    void setup();
    Vec4 insertGlyph(RasterizedGlyph& glyph);

private:
    int offset_x = 0;
    int offset_y = 0;
    int tallest = 0;
};
