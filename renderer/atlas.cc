#include "atlas.h"

namespace renderer {
void Atlas::setup(bool use_bilinear_filtering) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);

    GLint param = use_bilinear_filtering ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.
}

Vec4 Atlas::insertTexture(int width, int height, bool colored, GLubyte* data) {
    tallest = std::max(height, tallest);
    if (offset_x + width > ATLAS_SIZE) {
        offset_x = 0;
        offset_y += tallest;
        tallest = 0;
    }

    glBindTexture(GL_TEXTURE_2D, tex_id);

    GLenum format = colored ? GL_RGBA : GL_RGB;
    glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, width, height, format, GL_UNSIGNED_BYTE,
                    data);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    float uv_left = static_cast<float>(offset_x) / ATLAS_SIZE;
    float uv_bot = static_cast<float>(offset_y) / ATLAS_SIZE;
    float uv_width = static_cast<float>(width) / ATLAS_SIZE;
    float uv_height = static_cast<float>(height) / ATLAS_SIZE;

    offset_x += width;

    return Vec4{uv_left, uv_bot, uv_width, uv_height};
}

Atlas::~Atlas() {
    glDeleteTextures(1, &tex_id);
}
}
