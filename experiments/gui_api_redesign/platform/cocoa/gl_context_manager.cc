#include "gl_context_manager.h"

GLContextManager::~GLContextManager() {
    if (shared_context_) CGLReleaseContext(shared_context_);
    if (pixel_format_) CGLReleasePixelFormat(pixel_format_);
}

std::unique_ptr<GLContextManager> GLContextManager::create() {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFAOpenGLProfile,       (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAColorSize,           (CGLPixelFormatAttribute)24,
        kCGLPFADoubleBuffer,        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0,
    };

    CGLPixelFormatObj pf = nullptr;
    CGLContextObj ctx = nullptr;
    GLint n = 0;

    if (CGLChoosePixelFormat(attribs, &pf, &n) != kCGLNoError || !pf) {
        return nullptr;
    }
    if (CGLCreateContext(pf, nullptr, &ctx) != kCGLNoError || !ctx) {
        CGLReleasePixelFormat(pf);
        return nullptr;
    }

    auto mgr = std::unique_ptr<GLContextManager>(new GLContextManager());
    mgr->pixel_format_ = pf;
    mgr->shared_context_ = ctx;
    return mgr;
}

CGLContextObj GLContextManager::create_layer_context() const {
    CGLContextObj ctx;
    CGLCreateContext(pixel_format_, shared_context_, &ctx);
    return ctx;
}
