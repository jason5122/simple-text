#include "gl/loader.h"
// #include "base/windows/win32_error.h"
#include <cstdlib>
#include <windows.h>

namespace gl::internal {

// TODO: Handle errors.
void* load_proc_address(const char* fp) {
    static HMODULE module = LoadLibraryExA("opengl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    // https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows
    // `wglGetProcAddress()` returns functions up to OpenGL 1.1.
    // `GetProcAddress()` returns functions beyond OpenGL 1.1.
    // Therefore, we need to try using both functions.
    void* p = (void*)wglGetProcAddress(fp);
    if (!p) {
        p = (void*)GetProcAddress(module, fp);

        DWORD error = GetLastError();
        if (error != ERROR_SUCCESS) {
            // std::string error_str = base::windows::get_last_error_as_string(error);
            // spdlog::error("Error loading function `{}`: {}", fp, error_str);
            std::abort();
        }
    }
    return p;
}

}  // namespace gl::internal
