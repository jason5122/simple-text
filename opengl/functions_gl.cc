#include "functions_gl.h"
#include <dlfcn.h>

namespace opengl {

FunctionsGL::FunctionsGL(void* dylib_handle) : dylib_handle_(dylib_handle) {}

FunctionsGL::~FunctionsGL() {
    dlclose(dylib_handle_);
}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    return dlsym(dylib_handle_, function.c_str());
}

void FunctionsGL::initialize() {
    initProcsGL();
}

}
