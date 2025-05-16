#include "functions_gl.h"

#include <dlfcn.h>

namespace opengl_redesign::internal {

void* load_proc_address(const char* fp) {
    constexpr const char* kDylibPath =
        "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
    static void* handle = dlopen(kDylibPath, RTLD_NOW);
    return dlsym(handle, fp);
}

}  // namespace opengl_redesign::internal
