#include "gl/loader.h"
#include <dlfcn.h>

namespace gl::internal {

// TODO: Handle errors.
void* load_proc_address(const char* fp) {
    static void* handle =
        dlopen("/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib", RTLD_NOW);
    return dlsym(handle, fp);
}

}  // namespace gl::internal
