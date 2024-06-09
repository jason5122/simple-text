#include "gui/functions_gl.h"
#include <dlfcn.h>

#include <iostream>

namespace {
const char* kDefaultOpenGLDylibName =
    "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
}

// TODO: Properly organize these typedefs.
#ifndef INTERNAL_GL_APIENTRY
#ifdef ANGLE_PLATFORM_WINDOWS
#define INTERNAL_GL_APIENTRY __stdcall
#else
#define INTERNAL_GL_APIENTRY
#endif
#endif
typedef unsigned int GLbitfield;
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARPROC)(GLbitfield);
PFNGLCLEARPROC clear = nullptr;
#define GL_COLOR_BUFFER_BIT 0x00004000

namespace gui {

void FunctionsGL::initialize() {
    void* handle = dlopen(kDefaultOpenGLDylibName, RTLD_NOW);
    if (!handle) {
        std::cerr << "Could not open the OpenGL Framework.\n";
    }

    clear = reinterpret_cast<decltype(clear)>(dlsym(handle, "glClear"));

    clear(GL_COLOR_BUFFER_BIT);
}

}
