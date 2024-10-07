#include "display_gl.h"
#include "util/std_print.h"

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

namespace app {

DisplayGL::DisplayGL(CGLPixelFormatObj pixel_format, CGLContextObj context)
    : pixel_format_(pixel_format), context_(context) {}

DisplayGL::~DisplayGL() {}

std::unique_ptr<DisplayGL> DisplayGL::Create() {
    CGLPixelFormatObj pixel_format;
    CGLContextObj context;

    CGLPixelFormatAttribute attribs[] = {
        kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        kCGLPFAAllowOfflineRenderers,
        static_cast<CGLPixelFormatAttribute>(0),
    };

    GLint nVirtualScreens = 0;

    CGLChoosePixelFormat(attribs, &pixel_format, &nVirtualScreens);
    if (pixel_format == nullptr) {
        std::println("Could not create the context's pixel format.");
        return nullptr;
    }

    CGLCreateContext(pixel_format, nullptr, &context);
    if (pixel_format == nullptr) {
        std::println("Could not create the CGL context.");
        return nullptr;
    }

    if (CGLSetCurrentContext(context) != kCGLNoError) {
        std::println("Could not make the CGL context current.");
        return nullptr;
    }

    return std::unique_ptr<DisplayGL>(new DisplayGL(pixel_format, context));
}

CGLPixelFormatObj DisplayGL::pixelFormat() {
    return pixel_format_.get();
}

CGLContextObj DisplayGL::context() {
    return context_.get();
}

}
