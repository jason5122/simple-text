#include "gl/egl_types.h"
#include "gl/loader.h"
#include <dlfcn.h>

namespace gl::internal {

// TODO: Handle errors.
void* load_proc_address(const char* fp) {
    static void* handle = dlopen("libEGL.so.1", RTLD_NOW);
    static PFNEGLGETPROCADDRESSPROC mGetProcAddressPtr =
        reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(dlsym(handle, "eglGetProcAddress"));
    void* p = reinterpret_cast<void*>(mGetProcAddressPtr(fp));
    if (!p) p = dlsym(handle, fp);
    return p;
}

}  // namespace gl::internal
