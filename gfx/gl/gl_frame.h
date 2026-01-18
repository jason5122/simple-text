#pragma once

#include "gfx/frame.h"
#include "gfx/gl/gl_device.h"
#include "gfx/gl/gl_surface.h"
#include <vector>

namespace gfx {

class GLFrame final : public Frame {
public:
    GLFrame(GLDevice& device, GLSurface& surface) : device_(device), surface_(surface) {}

    void clear(const Color& c) override;
    void set_viewport(int width, int height) override;
    void draw_quads(std::span<const Quad> quads, float transform_x, float transform_y) override;
    void present() override;

private:
    GLDevice& device_;
    GLSurface& surface_;

    std::vector<Vertex> scratch_;
};

}  // namespace gfx
