#include "functions_gl.h"
#include "opengl/gl.h"

namespace opengl {

#define ASSIGN(NAME, FP) FP = reinterpret_cast<decltype(FP)>(loadProcAddress(NAME))

void FunctionsGL::loadGlobalFunctionPointers() {
    // 1.0
    ASSIGN("glClear", glClear);
    ASSIGN("glClearColor", glClearColor);
    ASSIGN("glDepthMask", glDepthMask);
    ASSIGN("glEnable", glEnable);
    ASSIGN("glGetError", glGetError);
    ASSIGN("glViewport", glViewport);
    ASSIGN("glPixelStorei", glPixelStorei);
    ASSIGN("glTexImage2D", glTexImage2D);
    ASSIGN("glTexParameteri", glTexParameteri);
    ASSIGN("glBlendFunc", glBlendFunc);

    // 1.1
    ASSIGN("glGenTextures", glGenTextures);
    ASSIGN("glDeleteTextures", glDeleteTextures);
    ASSIGN("glBindTexture", glBindTexture);
    ASSIGN("glTexSubImage2D", glTexSubImage2D);

    // 1.3
    ASSIGN("glActiveTexture", glActiveTexture);

    // 1.4
    ASSIGN("glBlendFuncSeparate", glBlendFuncSeparate);

    // 1.5
    ASSIGN("glGenBuffers", glGenBuffers);
    ASSIGN("glBindBuffer", glBindBuffer);
    ASSIGN("glBufferData", glBufferData);
    ASSIGN("glBufferSubData", glBufferSubData);
    ASSIGN("glDeleteBuffers", glDeleteBuffers);

    // 2.0
    ASSIGN("glAttachShader", glAttachShader);
    ASSIGN("glCompileShader", glCompileShader);
    ASSIGN("glCreateProgram", glCreateProgram);
    ASSIGN("glCreateShader", glCreateShader);
    ASSIGN("glDeleteProgram", glDeleteProgram);
    ASSIGN("glDeleteShader", glDeleteShader);
    ASSIGN("glLinkProgram", glLinkProgram);
    ASSIGN("glShaderSource", glShaderSource);
    ASSIGN("glGetShaderInfoLog", glGetShaderInfoLog);
    ASSIGN("glGetProgramInfoLog", glGetProgramInfoLog);
    ASSIGN("glGetShaderiv", glGetShaderiv);
    ASSIGN("glGetProgramiv", glGetProgramiv);
    ASSIGN("glEnableVertexAttribArray", glEnableVertexAttribArray);
    ASSIGN("glVertexAttribPointer", glVertexAttribPointer);
    ASSIGN("glUseProgram", glUseProgram);
    ASSIGN("glGetUniformLocation", glGetUniformLocation);
    ASSIGN("glUniform2f", glUniform2f);
    ASSIGN("glUniform1f", glUniform1f);
    ASSIGN("glUniform1i", glUniform1i);

    // 3.0
    ASSIGN("glBindVertexArray", glBindVertexArray);
    ASSIGN("glGenVertexArrays", glGenVertexArrays);
    ASSIGN("glDeleteVertexArrays", glDeleteVertexArrays);
    ASSIGN("glVertexAttribIPointer", glVertexAttribIPointer);

    // 3.1
    ASSIGN("glDrawElementsInstanced", glDrawElementsInstanced);

    // 3.3
    ASSIGN("glVertexAttribDivisor", glVertexAttribDivisor);
}

}
