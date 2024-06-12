#include "dispatch_table_gl.h"

namespace opengl {

#define ASSIGN(NAME, FP) FP = reinterpret_cast<decltype(FP)>(loadProcAddress(NAME))

void DispatchTableGL::initProcsGL() {
    ASSIGN("glClear", clear);
    ASSIGN("glClearColor", clearColor);
    ASSIGN("glDepthMask", depthMask);
    ASSIGN("glEnable", enable);
    ASSIGN("glGetError", getError);
    ASSIGN("glGenTextures", genTextures);
}

}
