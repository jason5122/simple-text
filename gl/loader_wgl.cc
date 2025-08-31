#include "gl/loader.h"
// #include "base/windows/win32_error.h"
#include <cstdlib>
#include <windows.h>

namespace gl::internal {

void* load_proc_address(const char* fp) {
    static HMODULE module = LoadLibraryExA("opengl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    // https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows
    // `wglGetProcAddress()` returns functions up to OpenGL 1.1.
    // `GetProcAddress()` returns functions beyond OpenGL 1.1.
    // Therefore, we need to try using both functions.
    void* p = (void*)wglGetProcAddress(fp);
    if (!p) p = (void*)GetProcAddress(module, fp);
    return p;
}

}  // namespace gl::internal
