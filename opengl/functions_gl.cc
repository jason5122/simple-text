#include "functions_gl.h"
#include <dlfcn.h>

#include <iostream>

namespace {
const char* kDefaultOpenGLDylibName =
    "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
}

namespace opengl {

FunctionsGL::FunctionsGL() {
    handle_ = dlopen(kDefaultOpenGLDylibName, RTLD_NOW);
    if (!handle_) {
        std::cerr << "Could not open the OpenGL Framework.\n";
    }
}

FunctionsGL::~FunctionsGL() {
    dlclose(handle_);
}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    return dlsym(handle_, function.c_str());
}

void FunctionsGL::initialize() {
    initProcsGL();
}

}
