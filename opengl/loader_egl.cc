#include "opengl/egl_types.h"
#include "opengl/loader.h"
#include <dlfcn.h>

namespace opengl::internal {

// TODO: Handle errors.
void* load_proc_address(const char* fp) {
    constexpr const char* kDylibPath = "libEGL.so.1";
    static void* handle = dlopen(kDylibPath, RTLD_NOW);
    static PFNEGLGETPROCADDRESSPROC mGetProcAddressPtr =
        reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(dlsym(handle, "eglGetProcAddress"));
    void* p = reinterpret_cast<void*>(mGetProcAddressPtr(fp));
    if (!p) {
        p = dlsym(handle, fp);
    }
    return p;
}

}  // namespace opengl::internal
