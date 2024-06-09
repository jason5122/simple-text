#include "gui/functions_gl.h"
#include "gui/gl/functionsgl_enums.h"
#include "gui/gl/functionsgl_typedefs.h"
#include <dlfcn.h>

#include <iostream>

namespace {
const char* kDefaultOpenGLDylibName =
    "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
}

namespace gui {

void FunctionsGL::initialize() {
    void* handle = dlopen(kDefaultOpenGLDylibName, RTLD_NOW);
    if (!handle) {
        std::cerr << "Could not open the OpenGL Framework.\n";
    }

    clear = reinterpret_cast<decltype(clear)>(dlsym(handle, "glClear"));
}

}
