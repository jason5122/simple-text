#include "functions_gl.h"
#include "opengl/egl_types.h"
#include <dlfcn.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace {

const char* kDefaultEGLPath = "libEGL.so.1";

}

namespace opengl {

class FunctionsGL::impl {
public:
    void* handle;
    PFNEGLGETPROCADDRESSPROC mGetProcAddressPtr;
};

FunctionsGL::FunctionsGL() : pimpl{new impl{}} {
    pimpl->handle = dlopen(kDefaultEGLPath, RTLD_NOW);
    if (!pimpl->handle) {
        std::println("Could not dlopen native EGL.");
        std::abort();
    }

    pimpl->mGetProcAddressPtr =
        reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(dlsym(pimpl->handle, "eglGetProcAddress"));
    if (!pimpl->mGetProcAddressPtr) {
        std::println("Could not find eglGetProcAddress");
    }
}

FunctionsGL::~FunctionsGL() {
    dlclose(pimpl->handle);
}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    void* p = reinterpret_cast<void*>(pimpl->mGetProcAddressPtr(function.c_str()));
    if (!p) {
        p = dlsym(pimpl->handle, function.c_str());
    }
    return p;
}

}
