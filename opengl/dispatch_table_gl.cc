#include "dispatch_table_gl.h"

namespace opengl {

#define ASSIGN(NAME, FP) FP = reinterpret_cast<decltype(FP)>(loadProcAddress(NAME))

void DispatchTableGL::initProcsGL() {
    // 1.0
    ASSIGN("glClear", clear);
    ASSIGN("glClearColor", clearColor);
    ASSIGN("glDepthMask", depthMask);
    ASSIGN("glEnable", enable);
    ASSIGN("glGetError", getError);

    // 1.1
    ASSIGN("glGenTextures", genTextures);

    // 2.0
    ASSIGN("glAttachShader", attachShader);
    ASSIGN("glCompileShader", compileShader);
    ASSIGN("glCreateProgram", createProgram);
    ASSIGN("glCreateShader", createShader);
    ASSIGN("glDeleteProgram", deleteProgram);
    ASSIGN("glDeleteShader", deleteShader);
    ASSIGN("glLinkProgram", linkProgram);
    ASSIGN("glShaderSource", shaderSource);
    ASSIGN("glGetShaderInfoLog", getShaderInfoLog);
    ASSIGN("glGetShaderiv", getShaderiv);
}

}
