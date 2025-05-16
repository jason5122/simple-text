#include "functions_gl.h"

#include "experiments/opengl_loader_redesign/egl_types.h"

#include <dlfcn.h>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace {

const char* kDefaultEGLPath = "libEGL.so.1";

}

namespace opengl_redesign {

class FunctionsGL::impl {
public:
    void* handle;
    PFNEGLGETPROCADDRESSPROC mGetProcAddressPtr;
};

FunctionsGL::FunctionsGL() : pimpl{new impl{}} {
    pimpl->handle = dlopen(kDefaultEGLPath, RTLD_NOW);
    if (!pimpl->handle) {
        fmt::println("Could not dlopen native EGL.");
        std::abort();
    }

    pimpl->mGetProcAddressPtr =
        reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(dlsym(pimpl->handle, "eglGetProcAddress"));
    if (!pimpl->mGetProcAddressPtr) {
        fmt::println("Could not find eglGetProcAddress");
    }
}

FunctionsGL::~FunctionsGL() { dlclose(pimpl->handle); }

void* FunctionsGL::load_proc_address(std::string_view function) const {
    void* p = reinterpret_cast<void*>(pimpl->mGetProcAddressPtr(function.data()));
    if (!p) {
        p = dlsym(pimpl->handle, function.data());
    }
    return p;
}

}  // namespace opengl_redesign
