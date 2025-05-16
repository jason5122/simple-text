#include "functions_gl.h"

#include <dlfcn.h>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace {

const char* kDefaultOpenGLDylibName =
    "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";

}

namespace opengl_redesign {

class FunctionsGL::impl {
public:
    void* handle;
};

FunctionsGL::FunctionsGL() : pimpl{new impl{}} {
    pimpl->handle = dlopen(kDefaultOpenGLDylibName, RTLD_NOW);
    if (!pimpl->handle) {
        fmt::println("Could not open the OpenGL Framework.");
        std::abort();
    }
}

FunctionsGL::~FunctionsGL() { dlclose(pimpl->handle); }

void* FunctionsGL::load_proc_address(std::string_view function) const {
    return dlsym(pimpl->handle, function.data());
}

}  // namespace opengl_redesign
