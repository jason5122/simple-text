#pragma once

#include "gfx/texture.h"
#include "gl/gl.h"
#include <cstdint>
#include <span>

namespace gfx {

class GLTexture final : public Texture {
public:
    GLTexture(int width, int height, TextureFormat format, std::span<const uint8_t> pixels);
    ~GLTexture() override;

    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    int width() const override { return width_; }
    int height() const override { return height_; }
    void update_region(int x,
                       int y,
                       int w,
                       int h,
                       TextureFormat format,
                       std::span<const uint8_t> pixels) override;

    gl::GLuint id() const { return id_; }

private:
    int width_ = 0;
    int height_ = 0;
    gl::GLuint id_ = 0;
};

}  // namespace gfx
