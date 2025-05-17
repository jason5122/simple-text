#include "base/windows/win32_error.h"
#include "opengl/functions_gl.h"
#include <fmt/fmt.base>
#include <windows.h>

namespace opengl::internal {

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
            std::string error_str = base::windows::GetLastErrorAsString(error);
            fmt::println("Error loading function `{}`: {}", fp, error_str);
            std::abort();
        }
    }
    return p;
}

}  // namespace opengl::internal
