#include "Atlas.h"
#include "util/LogUtil.h"

Atlas::Atlas(int32_t size) : width(size), height(size) {
    GLuint tex_id;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Glyph Atlas::insert_inner(RasterizedGlyph& glyph) {
    int32_t offset_x = this->row_extent;
    int32_t offset_y = this->row_baseline;

    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, glyph.width, glyph.height, GL_RGB,
                    GL_UNSIGNED_BYTE, &glyph.buffer);

    glBindTexture(GL_TEXTURE_2D, 0);

    row_extent = offset_x + glyph.width;
    if (glyph.height > row_tallest) {
        row_tallest = glyph.height;
    }

    float uv_bot = static_cast<float>(offset_y) / this->height;
    float uv_left = static_cast<float>(offset_x) / this->width;
    float uv_width = static_cast<float>(glyph.width) / this->width;
    float uv_height = static_cast<float>(glyph.height) / this->height;

    logDefault(@"Atlas", @"%d %d %d %d %d %f %f %f %f", tex_id, static_cast<int16_t>(glyph.top),
               static_cast<int16_t>(glyph.left), static_cast<int16_t>(glyph.width),
               static_cast<int16_t>(glyph.height), uv_bot, uv_left, uv_width, uv_height);

    return Glyph{tex_id,
                 static_cast<int16_t>(glyph.top),
                 static_cast<int16_t>(glyph.left),
                 static_cast<int16_t>(glyph.width),
                 static_cast<int16_t>(glyph.height),
                 uv_bot,
                 uv_left,
                 uv_width,
                 uv_height};
}
