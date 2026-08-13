#include "experiments/rasterizer/atlas.h"
#include <algorithm>

Atlas::Atlas() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_id_);
    glBindTexture(GL_TEXTURE_2D, tex_id_);

    // Zero-initialize so the untouched region reads as transparent -- keeps the atlas debug view
    // clean instead of showing whatever was in texture memory.
    std::vector<uint8_t> zeros(static_cast<size_t>(kSize) * kSize * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 zeros.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Atlas::~Atlas() { glDeleteTextures(1, &tex_id_); }

bool Atlas::insert(int width, int height, const std::vector<uint8_t>& pixels, UV& out_uv) {
    if (width > kSize || height > kSize) return false;
    if (!fits_in_row(width, height)) {
        if (!advance_row() || !fits_in_row(width, height)) return false;
    }

    glBindTexture(GL_TEXTURE_2D, tex_id_);
    // Premultiplied-first + host byte order == BGRA bytes on little-endian; this format/type pair
    // reads back as RGBA in the shader, matching the rasterizer's Core Graphics output.
    glTexSubImage2D(GL_TEXTURE_2D, 0, row_x_, row_y_, width, height, GL_BGRA,
                    GL_UNSIGNED_INT_8_8_8_8_REV, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    out_uv = {
        static_cast<float>(row_x_) / kSize,
        static_cast<float>(row_y_) / kSize,
        static_cast<float>(width) / kSize,
        static_cast<float>(height) / kSize,
    };

    row_x_ += width;
    row_height_ = std::max(row_height_, height);
    return true;
}

bool Atlas::fits_in_row(int width, int height) const {
    return row_x_ + width <= kSize && row_y_ + height <= kSize;
}

bool Atlas::advance_row() {
    row_y_ += row_height_;
    row_x_ = 0;
    row_height_ = 0;
    return row_y_ < kSize;
}
