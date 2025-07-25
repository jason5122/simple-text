#pragma once

#include "experiments/gui_api_redesign/platform/cocoa/gl_pixel_format.h"
#include <OpenGL/OpenGL.h>
#include <memory>

class GLContext {
public:
    ~GLContext();
    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;
    GLContext(GLContext&&);
    GLContext& operator=(GLContext&&);

    static std::unique_ptr<GLContext> create(const GLPixelFormat& pf);
    std::unique_ptr<GLContext> create_shared(const GLPixelFormat& pf) const;
    CGLContextObj get() const { return ctx_; }

private:
    GLContext(CGLContextObj ctx) : ctx_(ctx) {}
    CGLContextObj ctx_;
};
