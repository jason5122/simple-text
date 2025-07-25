#include "experiments/gui_api_redesign/platform/cocoa/gl_pixel_format.h"

GLPixelFormat::~GLPixelFormat() {
    if (pf_) CGLReleasePixelFormat(pf_);
}

GLPixelFormat::GLPixelFormat(GLPixelFormat&& other) : pf_(other.pf_) { other.pf_ = nullptr; }

GLPixelFormat& GLPixelFormat::operator=(GLPixelFormat&& other) {
    if (&other != this) {
        pf_ = other.pf_;
        other.pf_ = nullptr;
    }
    return *this;
}

std::unique_ptr<GLPixelFormat> GLPixelFormat::create() {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFAOpenGLProfile,       (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAColorSize,           (CGLPixelFormatAttribute)24,
        kCGLPFADoubleBuffer,        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0,
    };
    CGLPixelFormatObj pf;
    GLint n;
    if (CGLChoosePixelFormat(attribs, &pf, &n) != kCGLNoError || !pf) {
        return nullptr;
    }
    return std::unique_ptr<GLPixelFormat>(new GLPixelFormat(pf));
}
