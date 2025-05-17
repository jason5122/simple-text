#include "opengl/functions_gl.h"
#include <dlfcn.h>

namespace opengl::internal {

// TODO: Handle errors.
void* load_proc_address(const char* fp) {
    constexpr const char* kDylibPath =
        "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
    static void* handle = dlopen(kDylibPath, RTLD_NOW);
    return dlsym(handle, fp);
}

}  // namespace opengl::internal
