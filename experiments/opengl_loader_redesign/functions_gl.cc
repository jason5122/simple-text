#include "functions_gl.h"

#include "experiments/opengl_loader_redesign/gl.h"

namespace opengl_redesign {

#define ASSIGN(FP) FP = reinterpret_cast<decltype(FP)>(load_proc_address(#FP))

void FunctionsGL::load_global_function_pointers() {
    // 1.0
    ASSIGN(glClear);
    ASSIGN(glClearColor);
    ASSIGN(glDepthMask);
    ASSIGN(glEnable);
    ASSIGN(glGetError);
    ASSIGN(glGetIntegerv);
    ASSIGN(glViewport);
    ASSIGN(glPixelStorei);
    ASSIGN(glTexImage2D);
    ASSIGN(glTexParameteri);
    ASSIGN(glBlendFunc);
    ASSIGN(glScissor);
    ASSIGN(glDisable);

    // 1.1
    ASSIGN(glGenTextures);
    ASSIGN(glDeleteTextures);
    ASSIGN(glBindTexture);
    ASSIGN(glTexSubImage2D);

    // 1.3
    ASSIGN(glActiveTexture);

    // 1.4
    ASSIGN(glBlendFuncSeparate);

    // 1.5
    ASSIGN(glGenBuffers);
    ASSIGN(glBindBuffer);
    ASSIGN(glBufferData);
    ASSIGN(glBufferSubData);
    ASSIGN(glDeleteBuffers);

    // 2.0
    ASSIGN(glAttachShader);
    ASSIGN(glCompileShader);
    ASSIGN(glCreateProgram);
    ASSIGN(glCreateShader);
    ASSIGN(glDeleteProgram);
    ASSIGN(glDeleteShader);
    ASSIGN(glLinkProgram);
    ASSIGN(glShaderSource);
    ASSIGN(glGetShaderInfoLog);
    ASSIGN(glGetProgramInfoLog);
    ASSIGN(glGetShaderiv);
    ASSIGN(glGetProgramiv);
    ASSIGN(glEnableVertexAttribArray);
    ASSIGN(glVertexAttribPointer);
    ASSIGN(glUseProgram);
    ASSIGN(glGetUniformLocation);
    ASSIGN(glUniform2f);
    ASSIGN(glUniform1f);
    ASSIGN(glUniform1i);

    // 3.0
    ASSIGN(glBindVertexArray);
    ASSIGN(glGenVertexArrays);
    ASSIGN(glDeleteVertexArrays);
    ASSIGN(glVertexAttribIPointer);

    // 3.1
    ASSIGN(glDrawElementsInstanced);

    // 3.3
    ASSIGN(glVertexAttribDivisor);
}

}  // namespace opengl_redesign
