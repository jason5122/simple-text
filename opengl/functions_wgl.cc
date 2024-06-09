#include "functions_gl.h"
#include <windows.h>

namespace opengl {

FunctionsGL::FunctionsGL() {}

FunctionsGL::~FunctionsGL() {}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    // https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows
    // `wglGetProcAddress()` returns functions up to OpenGL 1.1.
    // `GetProcAddress()` returns functions beyond OpenGL 1.1.
    // Therefore, we need to try using both functions.
    void* p = (void*)wglGetProcAddress(function.c_str());
    if (!p) {
        HMODULE module = LoadLibraryExA("opengl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        p = (void*)GetProcAddress(module, function.c_str());
    }
    return p;
}

void FunctionsGL::initialize() {
    initProcsGL();
}

}
