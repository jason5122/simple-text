#include "atlas.h"

namespace renderer {
void Atlas::setup() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    // DEBUG: Color atlas background to spot incorrect shaders easier.
    // This helped with debugging the fractional pixel scrolling bug.
    // TODO: Creating the `data` vector is quite slow, so disable during release.
    data = std::vector<uint8_t>(kAtlasSize * kAtlasSize * 4, 0);
    size_t pixels = kAtlasSize * kAtlasSize;
    for (int i = 0; i < pixels; i++) {
        size_t offset = i * 4;
        data[offset + 2] = 0;
        data[offset + 1] = 255;
        data[offset] = 0;
        data[offset + 3] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kAtlasSize, kAtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 &data[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.
}

Vec4 Atlas::insertTexture(int width, int height, bool colored, GLubyte* data) {
    tallest = std::max(height, tallest);
    if (offset_x + width > kAtlasSize) {
        offset_x = 0;
        offset_y += tallest;
        tallest = 0;
    }

    glBindTexture(GL_TEXTURE_2D, tex_id);

    GLenum format = colored ? GL_RGBA : GL_RGB;
    glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, width, height, format, GL_UNSIGNED_BYTE,
                    data);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    float uv_left = static_cast<float>(offset_x) / kAtlasSize;
    float uv_bot = static_cast<float>(offset_y) / kAtlasSize;
    float uv_width = static_cast<float>(width) / kAtlasSize;
    float uv_height = static_cast<float>(height) / kAtlasSize;

    offset_x += width;

    return Vec4{uv_left, uv_bot, uv_width, uv_height};
}

Atlas::~Atlas() {
    glDeleteTextures(1, &tex_id);
}
}
