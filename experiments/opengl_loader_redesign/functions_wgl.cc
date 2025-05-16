#include "functions_gl.h"

#include "base/windows/win32_error.h"

#include <windows.h>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace opengl_redesign {

class FunctionsGL::impl {
public:
    HMODULE module;
};

FunctionsGL::FunctionsGL() : pimpl{new impl{}} {
    pimpl->module = LoadLibraryExA("opengl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (!pimpl->module) {
        fmt::println("could not load opengl32.dll");
    }
}

FunctionsGL::~FunctionsGL() {}

void* FunctionsGL::load_proc_address(std::string_view function) const {
    // https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows
    // `wglGetProcAddress()` returns functions up to OpenGL 1.1.
    // `GetProcAddress()` returns functions beyond OpenGL 1.1.
    // Therefore, we need to try using both functions.
    void* p = (void*)wglGetProcAddress(function.data());
    if (!p) {
        p = (void*)GetProcAddress(pimpl->module, function.data());

        DWORD error = GetLastError();
        if (error != 0) {
            std::string error_str = GetLastErrorAsString(error);
            fmt::println("Error loading function `{}`: {}", function, error_str);
        }
    }
    return p;
}

}  // namespace opengl_redesign
