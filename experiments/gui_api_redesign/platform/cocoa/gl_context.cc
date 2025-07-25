#include "experiments/gui_api_redesign/platform/cocoa/gl_context.h"

GLContext::~GLContext() {
    if (ctx_) CGLReleaseContext(ctx_);
}

GLContext::GLContext(GLContext&& other) : ctx_(other.ctx_) { other.ctx_ = nullptr; }

GLContext& GLContext::operator=(GLContext&& other) {
    if (&other != this) {
        ctx_ = other.ctx_;
        other.ctx_ = nullptr;
    }
    return *this;
}

std::unique_ptr<GLContext> GLContext::create(const GLPixelFormat& pf) {
    CGLContextObj ctx;
    if (CGLCreateContext(pf.get(), nullptr, &ctx) != kCGLNoError || !ctx) {
        return nullptr;
    }
    return std::unique_ptr<GLContext>(new GLContext(ctx));
}

std::unique_ptr<GLContext> GLContext::create_shared(const GLPixelFormat& pf) const {
    CGLContextObj ctx;
    if (CGLCreateContext(pf.get(), ctx_, &ctx) != kCGLNoError || !ctx) {
        return nullptr;
    }
    return std::unique_ptr<GLContext>(new GLContext(ctx));
}
