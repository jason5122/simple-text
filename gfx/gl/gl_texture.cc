#include "gfx/gl/gl_texture.h"
using namespace gl;

namespace gfx {

namespace {

GLenum to_gl_format(TextureFormat format) {
    switch (format) {
        case TextureFormat::kRGBA8:
            return GL_RGBA;
        case TextureFormat::kBGRA8:
            return GL_BGRA;
    }
    return GL_RGBA;
}

}  // namespace

GLTexture::GLTexture(int width, int height, TextureFormat format, std::span<const uint8_t> pixels)
    : width_(width), height_(height) {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    const void* data = pixels.empty() ? nullptr : pixels.data();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, to_gl_format(format),
                 GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

GLTexture::~GLTexture() {
    if (id_) glDeleteTextures(1, &id_);
}

void GLTexture::update_region(int x,
                              int y,
                              int w,
                              int h,
                              TextureFormat format,
                              std::span<const uint8_t> pixels) {
    if (w <= 0 || h <= 0 || pixels.empty()) return;

    glBindTexture(GL_TEXTURE_2D, id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, to_gl_format(format), GL_UNSIGNED_BYTE,
                    pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace gfx
