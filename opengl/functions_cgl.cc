#include "functions_gl.h"
#include <dlfcn.h>

#include <iostream>

namespace {
const char* kDefaultOpenGLDylibName =
    "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
}

namespace opengl {

class FunctionsGL::impl {
public:
    void* handle;
};

FunctionsGL::FunctionsGL() : pimpl{new impl{}} {
    pimpl->handle = dlopen(kDefaultOpenGLDylibName, RTLD_NOW);
    if (!pimpl->handle) {
        std::cerr << "Could not open the OpenGL Framework.\n";
    }
}

FunctionsGL::~FunctionsGL() {
    dlclose(pimpl->handle);
}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    return dlsym(pimpl->handle, function.c_str());
}

}
