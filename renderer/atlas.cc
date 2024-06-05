#include "atlas.h"

#include <iostream>

namespace renderer {

Atlas::~Atlas() {
    glDeleteTextures(1, &tex_id);
}

void Atlas::setup() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    // TODO: Incorporate this into the build system.
    const void* data = nullptr;

    bool debug_atlas = true;
    if (debug_atlas) {
        // DEBUG: Color atlas background to spot incorrect shaders easier.
        // This helped with debugging the fractional pixel scrolling bug.
        // TODO: Creating this` vector is quite slow, so disable during release.
        atlas_background = std::vector<uint8_t>(kAtlasSize * kAtlasSize * 4, 0);
        size_t pixels = kAtlasSize * kAtlasSize;
        for (int i = 0; i < pixels; i++) {
            size_t offset = i * 4;
            atlas_background[offset + 2] = 0;
            atlas_background[offset + 1] = 255;
            atlas_background[offset] = 0;
            atlas_background[offset + 3] = 255;
        }

        data = &atlas_background[0];
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kAtlasSize, kAtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.
}

Vec4 Atlas::insertTexture(int width, int height, bool colored, GLubyte* data) {
    if (!roomInRow(width, height)) {
        bool success = advanceRow();
        if (!success) {
            std::cerr << "Atlas is full.\n";
        }
    }

    // Load data into OpenGL.
    glBindTexture(GL_TEXTURE_2D, tex_id);
    GLenum format = colored ? GL_RGBA : GL_RGB;
    glTexSubImage2D(GL_TEXTURE_2D, 0, row_extent, row_baseline, width, height, format,
                    GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    // Generate UV coordinates.
    float uv_left = static_cast<float>(row_extent) / kAtlasSize;
    float uv_bot = static_cast<float>(row_baseline) / kAtlasSize;
    float uv_width = static_cast<float>(width) / kAtlasSize;
    float uv_height = static_cast<float>(height) / kAtlasSize;

    // Update Atlas state.
    row_extent += width;
    row_tallest = std::max(height, row_tallest);

    return Vec4{uv_left, uv_bot, uv_width, uv_height};
}

bool Atlas::roomInRow(int width, int height) {
    int next_extent = row_extent + width;
    bool enough_width = next_extent <= kAtlasSize;
    bool enough_height = height < (kAtlasSize - row_baseline);

    return enough_width && enough_height;
}

bool Atlas::advanceRow() {
    int advance_to = row_baseline + row_tallest;
    if (kAtlasSize - advance_to <= 0) {
        return false;  // Atlas is full.
    }

    row_baseline = advance_to;
    row_extent = 0;
    row_tallest = 0;
    return true;
}

}
