#include "displaygl.h"
#import <Cocoa/Cocoa.h>
#include <glad/glad.h>
#include <iostream>

DisplayGL::DisplayGL() : mContext(nullptr), mPixelFormat(nullptr) {}

DisplayGL::~DisplayGL() {
    CGLDestroyContext(mContext);
    CGLDestroyPixelFormat(mPixelFormat);
}

bool DisplayGL::initialize() {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        kCGLPFAAllowOfflineRenderers,
        static_cast<CGLPixelFormatAttribute>(0),
    };

    GLint nVirtualScreens = 0;

    CGLChoosePixelFormat(attribs, &mPixelFormat, &nVirtualScreens);
    if (mPixelFormat == nullptr) {
        std::cerr << "Could not create the context's pixel format." << '\n';
        return false;
    }

    CGLCreateContext(mPixelFormat, nullptr, &mContext);
    if (mPixelFormat == nullptr) {
        std::cerr << "Could not create the CGL context." << '\n';
        return false;
    }

    if (CGLSetCurrentContext(mContext) != kCGLNoError) {
        std::cerr << "Could not make the CGL context current." << '\n';
        return false;
    }

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD.\n";
        return false;
    }

    return true;
}
