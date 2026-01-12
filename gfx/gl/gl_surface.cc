#include "gfx/gl/gl_frame.h"
#include "gfx/gl/gl_surface.h"

namespace gfx {

std::unique_ptr<Frame> GLSurface::begin_frame() {
    return std::make_unique<GLFrame>(device_, *this);
}

void GLSurface::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

}  // namespace gfx
