#pragma once

#include "gfx/gl/gl_device.h"
#include "gfx/surface.h"
#include <memory>

namespace gfx {

class GLSurface final : public Surface {
public:
    GLSurface(GLDevice& device, int width, int height)
        : device_(device), width_(width), height_(height) {}

    std::unique_ptr<Frame> begin_frame() override;
    void resize(int width, int height) override;
    int width() const override { return width_; }
    int height() const override { return height_; }

private:
    GLDevice& device_;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace gfx
