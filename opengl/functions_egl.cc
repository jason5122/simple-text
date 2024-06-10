#include "functions_gl.h"
#include "opengl/functionsegl_typedefs.h"
#include <dlfcn.h>

#include <iostream>

namespace {
const char* kDefaultEGLPath = "libEGL.so.1";
}

namespace opengl {

FunctionsGL::FunctionsGL() {
    handle_ = dlopen(kDefaultEGLPath, RTLD_NOW);
    if (!handle_) {
        std::cerr << "Could not dlopen native EGL.\n";
    }
}

FunctionsGL::~FunctionsGL() {
    dlclose(handle_);
}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    // TODO: Fetch this function pointer only once. Move this into PIMPL idiom.
    PFNEGLGETPROCADDRESSPROC mGetProcAddressPtr =
        reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(dlsym(handle_, "eglGetProcAddress"));
    if (!mGetProcAddressPtr) {
        std::cerr << "Could not find eglGetProcAddress\n";
    }

    void* p = reinterpret_cast<void*>(mGetProcAddressPtr(function.c_str()));
    if (!p) {
        p = dlsym(handle_, function.c_str());
    }
    return p;
}

void FunctionsGL::initialize() {
    initProcsGL();
}

}
