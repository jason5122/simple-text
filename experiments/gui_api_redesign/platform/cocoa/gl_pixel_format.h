#pragma once

#include <OpenGL/OpenGL.h>
#include <memory>

class GLPixelFormat {
public:
    ~GLPixelFormat();
    GLPixelFormat(const GLPixelFormat&) = delete;
    GLPixelFormat& operator=(const GLPixelFormat&) = delete;
    GLPixelFormat(GLPixelFormat&&);
    GLPixelFormat& operator=(GLPixelFormat&&);

    static std::unique_ptr<GLPixelFormat> create();
    CGLPixelFormatObj get() const { return pf_; }

private:
    GLPixelFormat(CGLPixelFormatObj pf) : pf_(pf) {}
    CGLPixelFormatObj pf_;
};
